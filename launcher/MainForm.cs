using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.Json.Nodes;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace GodjoLauncher;

public sealed class MainForm : Form
{
    private readonly WebView2 _web = new() { Dock = DockStyle.Fill, DefaultBackgroundColor = Color.FromArgb(14, 15, 20) };
    private readonly Config _cfg = Config.Load();
    private readonly LauncherSettings _set = LauncherSettings.Load();
    private readonly Updater _upd;
    private bool _busy;

    [DllImport("user32.dll")] private static extern bool ReleaseCapture();
    [DllImport("user32.dll")] private static extern IntPtr SendMessage(IntPtr h, int msg, int wp, int lp);

    public MainForm()
    {
        _upd = new Updater(_set);
        Text = "Godjo Role Play";
        FormBorderStyle = FormBorderStyle.None;
        StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(1080, 640);
        BackColor = Color.FromArgb(14, 15, 20);
        try { using var s = typeof(MainForm).Assembly.GetManifestResourceStream("app.ico"); if (s != null) Icon = new Icon(s); } catch { }
        Controls.Add(_web);
        Load += async (_, _) => await InitAsync();
        if (string.IsNullOrEmpty(_cfg.GtaPath)) _cfg.GtaPath = Gta.GuessPath();
        if (string.IsNullOrEmpty(_cfg.Nick)) _cfg.Nick = Gta.GetNick();
    }

    private async Task InitAsync()
    {
        try
        {
            var env = await CoreWebView2Environment.CreateAsync(null, Path.Combine(Config.Dir, "webview"));
            await _web.EnsureCoreWebView2Async(env);
        }
        catch (Exception)
        {
            var r = MessageBox.Show("Для работы лаунчера нужен компонент Microsoft Edge WebView2 Runtime.\nОткрыть страницу загрузки?", "Godjo Role Play", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
            if (r == DialogResult.Yes) OpenUrl("https://go.microsoft.com/fwlink/p/?LinkId=2124703");
            Close();
            return;
        }
        var core = _web.CoreWebView2;
        core.Settings.AreDevToolsEnabled = false;
        core.Settings.AreDefaultContextMenusEnabled = false;
        core.Settings.IsStatusBarEnabled = false;
        core.Settings.IsZoomControlEnabled = false;
        core.WebMessageReceived += async (_, e) => { try { await OnMessage(e.WebMessageAsJson); } catch (Exception ex) { Send("error", new { text = ex.Message }); } };
        using var st = typeof(MainForm).Assembly.GetManifestResourceStream("ui.index.html")!;
        using var rd = new StreamReader(st);
        core.NavigateToString(await rd.ReadToEndAsync());
    }

    private void Send(string type, object data)
    {
        var json = JsonSerializer.Serialize(new { type, data });
        if (InvokeRequired) BeginInvoke(() => _web.CoreWebView2?.PostWebMessageAsJson(json));
        else _web.CoreWebView2?.PostWebMessageAsJson(json);
    }

    private (string ip, int port) ServerAddr()
    {
        if (!string.IsNullOrEmpty(_cfg.ServerIp) && _cfg.ServerPort > 0) return (_cfg.ServerIp, _cfg.ServerPort);
        var ms = _upd.Current?.Server;
        if (ms != null && !string.IsNullOrEmpty(ms.Ip)) return (ms.Ip, ms.Port > 0 ? ms.Port : 7777);
        return (_set.ServerIp, _set.ServerPort);
    }

    private object State()
    {
        var chk = Gta.Check(_cfg.GtaPath);
        var pend = chk.HasGta ? _upd.Pending(_cfg.GtaPath).Select(p => string.IsNullOrEmpty(p.Title) ? p.Name : p.Title).ToList() : new List<string>();
        var (ip, port) = ServerAddr();
        return new
        {
            gta = _cfg.GtaPath, nick = _cfg.Nick, check = chk, pending = pend, manifest = _upd.Current != null,
            version = _upd.Current?.Version ?? "", server = new { name = _upd.Current?.Server?.Name ?? _set.ServerName, ip, port },
            customIp = _cfg.ServerIp, customPort = _cfg.ServerPort, closeOnPlay = _cfg.CloseOnPlay, site = _set.Site,
            launcherVersion = Application.ProductVersion.Split('+')[0], launcherUpdate = _upd.Current?.Launcher?.Version is { Length: > 0 } lv && lv != Application.ProductVersion.Split('+')[0] ? _upd.Current.Launcher.Url : ""
        };
    }

    private async Task OnMessage(string raw)
    {
        var m = JsonNode.Parse(raw)!;
        string cmd = m["cmd"]?.GetValue<string>() ?? "";
        switch (cmd)
        {
            case "ready":
                Send("state", State());
                await _upd.LoadManifestAsync();
                Send("state", State());
                Send("news", _upd.Current?.News ?? new List<NewsItem>());
                _ = QueryLoop();
                break;
            case "drag":
                ReleaseCapture(); SendMessage(Handle, 0xA1, 0x2, 0); break;
            case "min": WindowState = FormWindowState.Minimized; break;
            case "close": Close(); break;
            case "browse":
            {
                using var d = new FolderBrowserDialog { Description = "Папка с GTA San Andreas (где лежит gta_sa.exe)", UseDescriptionForTitle = true, SelectedPath = _cfg.GtaPath };
                if (d.ShowDialog(this) == DialogResult.OK) { _cfg.GtaPath = d.SelectedPath; _cfg.Save(); }
                Send("state", State());
                break;
            }
            case "nick":
                _cfg.Nick = m["value"]?.GetValue<string>()?.Trim() ?? ""; _cfg.Save(); Send("state", State()); break;
            case "settings":
                _cfg.ServerIp = m["ip"]?.GetValue<string>()?.Trim() ?? "";
                _cfg.ServerPort = int.TryParse(m["port"]?.ToString(), out var pp) ? pp : 0;
                _cfg.CloseOnPlay = m["closeOnPlay"]?.GetValue<bool>() ?? true;
                _cfg.Save(); Send("state", State()); break;
            case "url": OpenUrl(m["value"]?.GetValue<string>() ?? ""); break;
            case "install": await Install(false); break;
            case "play": await Install(true); break;
            case "refresh":
                await _upd.LoadManifestAsync(); Send("state", State()); Send("news", _upd.Current?.News ?? new List<NewsItem>()); break;
        }
    }

    private async Task Install(bool thenPlay)
    {
        if (_busy) return;
        var chk = Gta.Check(_cfg.GtaPath);
        if (!chk.HasGta || !chk.HasSamp) { Send("error", new { text = chk.Message }); return; }
        if (!chk.SampSupported) { Send("error", new { text = chk.Message + ". Скачайте подходящий клиент SA-MP и установите его в папку игры." }); return; }
        if (thenPlay && !Gta.IsRpNick(_cfg.Nick)) { Send("error", new { text = "Ник должен быть в формате Имя_Фамилия латиницей (например, Ivan_Petrov)." }); return; }
        if (_upd.Current == null) await _upd.LoadManifestAsync();
        _busy = true;
        try
        {
            if (_upd.Current == null)
            {
                if (!File.Exists(Path.Combine(_cfg.GtaPath, "cef.asi"))) { Send("error", new { text = "Не удалось получить список файлов. Проверьте интернет или положите папку client рядом с лаунчером." }); return; }
            }
            else if (_upd.Pending(_cfg.GtaPath).Count > 0)
            {
                var prog = new Progress<(string text, double pct)>(p => Send("progress", new { p.text, p.pct }));
                await _upd.InstallAsync(_cfg.GtaPath, prog);
            }
            Send("state", State());
            if (thenPlay)
            {
                Gta.SetNick(_cfg.Nick);
                var (ip, port) = ServerAddr();
                Gta.Launch(_cfg.GtaPath, ip, port);
                Send("launched", new { });
                if (_cfg.CloseOnPlay) { await Task.Delay(1500); Close(); }
            }
        }
        catch (UnauthorizedAccessException)
        {
            Send("error", new { text = "Нет доступа к папке игры. Запустите лаунчер от имени администратора или перенесите GTA из Program Files." });
        }
        finally { _busy = false; Send("progress", new { text = "", pct = -1.0 }); }
    }

    private async Task QueryLoop()
    {
        while (!IsDisposed)
        {
            var (ip, port) = ServerAddr();
            var info = await SampQuery.QueryAsync(ip, port);
            Send("online", info);
            await Task.Delay(15000);
        }
    }

    private static void OpenUrl(string url)
    {
        if (!url.StartsWith("http://") && !url.StartsWith("https://")) return;
        try { System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(url) { UseShellExecute = true }); } catch { }
    }
}
