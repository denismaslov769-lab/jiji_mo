using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.Json.Nodes;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace GodjoLauncher;

public sealed class MainForm : Form
{
    // camelCase: интерфейс читает поля как check.ok / check.message (раньше уходили Ok/Message — из-за этого висел пустой «⚠»)
    private static readonly JsonSerializerOptions Json = new() { PropertyNamingPolicy = JsonNamingPolicy.CamelCase };

    private readonly WebView2 _web = new() { Dock = DockStyle.Fill, DefaultBackgroundColor = Color.FromArgb(14, 15, 20) };
    private readonly Config _cfg = Config.Load();
    private readonly LauncherSettings _set = LauncherSettings.Load();
    private readonly GameDownloader _game = new();
    private readonly Updater _upd;
    private CancellationTokenSource? _cts;
    private bool _busy;
    private ServerInfo? _lastInfo;

    [DllImport("user32.dll")] private static extern bool ReleaseCapture();
    [DllImport("user32.dll")] private static extern IntPtr SendMessage(IntPtr h, int msg, int wp, int lp);

    public MainForm()
    {
        _upd = new Updater(_set);
        Text = "Godjo Role Play";
        FormBorderStyle = FormBorderStyle.None;
        StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(1120, 680);
        MinimumSize = new Size(980, 620);
        BackColor = Color.FromArgb(14, 15, 20);
        try { using var s = typeof(MainForm).Assembly.GetManifestResourceStream("app.ico"); if (s != null) Icon = new Icon(s); } catch { }
        Controls.Add(_web);
        Load += async (_, _) => await InitAsync();
        if (string.IsNullOrEmpty(_cfg.GtaPath)) _cfg.GtaPath = Gta.GuessPath();
        if (string.IsNullOrEmpty(_cfg.Nick)) _cfg.Nick = Gta.GetNick();
        Log.Write($"start v{AppVersion}, gta='{_cfg.GtaPath}'");
    }

    private static string AppVersion => Application.ProductVersion.Split('+')[0];

    private async Task InitAsync()
    {
        try
        {
            var env = await CoreWebView2Environment.CreateAsync(null, Path.Combine(Config.Dir, "webview"));
            await _web.EnsureCoreWebView2Async(env);
        }
        catch (Exception ex)
        {
            Log.Write("webview2: " + ex.Message);
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
        core.WebMessageReceived += async (_, e) =>
        {
            try { await OnMessage(e.WebMessageAsJson); }
            catch (Exception ex) { Log.Write("ui: " + ex); Send("error", new { text = ex.Message }); }
        };
        using var st = typeof(MainForm).Assembly.GetManifestResourceStream("ui.index.html")!;
        using var rd = new StreamReader(st);
        core.NavigateToString(await rd.ReadToEndAsync());
    }

    private void Send(string type, object data)
    {
        var json = JsonSerializer.Serialize(new { type, data }, Json);
        void Post() { try { _web.CoreWebView2?.PostWebMessageAsJson(json); } catch { } }
        if (IsDisposed) return;
        if (InvokeRequired) BeginInvoke(Post); else Post();
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
        var pend = chk.HasGta ? _upd.Pending(_cfg.GtaPath, _cfg).Select(p => string.IsNullOrEmpty(p.Title) ? p.Name : p.Title).ToList() : new List<string>();
        var remove = chk.HasGta ? _upd.ToRemove(_cfg.GtaPath, _cfg).Select(p => p.Title).ToList() : new List<string>();
        long pendSize = chk.HasGta ? _upd.Pending(_cfg.GtaPath, _cfg).Sum(p => p.Size) : 0;
        var (ip, port) = ServerAddr();
        var lu = _upd.Current?.Launcher?.Version;
        return new
        {
            gta = _cfg.GtaPath, nick = _cfg.Nick, nickHistory = _cfg.NickHistory, check = chk, pending = pend, pendingSize = pendSize, remove,
            manifest = _upd.Current != null, version = _upd.Current?.Version ?? "",
            server = new { name = _upd.Current?.Server?.Name ?? _set.ServerName, ip, port },
            customIp = _cfg.ServerIp, customPort = _cfg.ServerPort, closeOnPlay = _cfg.CloseOnPlay,
            showLoading = _cfg.ShowLoadingScreen, sounds = _cfg.PlaySounds,
            site = _set.Site, discord = _set.Discord, vk = _set.Vk, telegram = _set.Telegram,
            exeVersion = chk.HasGta ? Gta.ExeVersion(_cfg.GtaPath) : "", freeMb = chk.HasGta ? Gta.FreeSpaceMb(_cfg.GtaPath) : -1,
            mods = _upd.Views(_cfg.GtaPath, _cfg), tips = _upd.Current?.Tips ?? new List<string>(),
            launcherVersion = AppVersion,
            launcherUpdate = lu is { Length: > 0 } && IsNewer(lu, AppVersion) ? _upd.Current!.Launcher!.Url : "",
            busy = _busy,
            game = _game.Pack == null ? null : new { title = _game.Pack.Title, size = _game.Pack.Size, unpacked = _game.Pack.Unpacked, version = _game.Pack.Version }
        };
    }

    private static bool IsNewer(string a, string b) =>
        Version.TryParse(a, out var va) && Version.TryParse(b, out var vb) ? va > vb : a != b;

    private async Task OnMessage(string raw)
    {
        var m = JsonNode.Parse(raw)!;
        string cmd = m["cmd"]?.GetValue<string>() ?? "";
        switch (cmd)
        {
            case "ready":
                Send("state", State());
                _ = LoadGamePackAsync();
                await _upd.LoadManifestAsync();
                Send("state", State());
                Send("news", _upd.Current?.News ?? new List<NewsItem>());
                _ = QueryLoop();
                break;
            case "drag": ReleaseCapture(); SendMessage(Handle, 0xA1, 0x2, 0); break;
            case "min": WindowState = FormWindowState.Minimized; break;
            case "close": _cts?.Cancel(); Close(); break;
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
                _cfg.ShowLoadingScreen = m["showLoading"]?.GetValue<bool>() ?? true;
                _cfg.PlaySounds = m["sounds"]?.GetValue<bool>() ?? true;
                _cfg.Save(); Send("state", State()); break;
            case "mod":
            {
                var name = m["name"]?.GetValue<string>() ?? "";
                var on = m["on"]?.GetValue<bool>() ?? true;
                if (_busy) { Send("error", new { text = "Дождитесь окончания установки." }); break; }
                _cfg.Mods[name] = on; _cfg.Save(); Send("state", State()); break;
            }
            case "url": OpenUrl(m["value"]?.GetValue<string>() ?? ""); break;
            case "openGame":
                if (Directory.Exists(_cfg.GtaPath)) System.Diagnostics.Process.Start("explorer.exe", $"\"{_cfg.GtaPath}\"");
                break;
            case "openLog":
                if (File.Exists(Log.FilePath)) System.Diagnostics.Process.Start("notepad.exe", $"\"{Log.FilePath}\"");
                break;
            case "copyIp":
            {
                var (ip, port) = ServerAddr();
                Clipboard.SetText($"{ip}:{port}");
                Send("toast", new { text = $"Адрес {ip}:{port} скопирован", ok = true });
                break;
            }
            case "verify":
            {
                if (!Gta.Check(_cfg.GtaPath).HasGta) { Send("error", new { text = "Сначала укажите папку с игрой." }); break; }
                int broken = _upd.Verify(_cfg.GtaPath);
                Send("toast", new { text = broken == 0 ? "Проверка завершена: все файлы на месте ✔" : $"Повреждено пакетов: {broken}. Нажмите «Установить», чтобы восстановить.", ok = broken == 0 });
                Send("state", State());
                break;
            }
            case "reinstall":
                Updater.ResetState(_cfg.GtaPath);
                await _upd.LoadManifestAsync(); Send("state", State());
                Send("toast", new { text = "Файлы будут скачаны заново при следующем запуске.", ok = true });
                break;
            case "cancel": _cts?.Cancel(); break;
            case "install": await Install(false); break;
            case "downloadGame": await DownloadGame(); break;
            case "play": await Install(true); break;
            case "refresh":
                await _upd.LoadManifestAsync(); Send("state", State()); Send("news", _upd.Current?.News ?? new List<NewsItem>()); break;
        }
    }

    private void Stage(string stage, string text, double pct, object? extra = null) =>
        Send("loading", new { stage, text, pct, extra });

    private async Task Install(bool thenPlay)
    {
        if (_busy) return;
        var chk = Gta.Check(_cfg.GtaPath);
        if (!chk.HasGta || !chk.HasSamp) { Send("error", new { text = chk.Message }); return; }
        if (!chk.SampSupported) { Send("error", new { text = chk.Message + ". Скачайте подходящий клиент SA-MP и установите его в папку игры." }); return; }
        if (thenPlay && !Gta.IsRpNick(_cfg.Nick)) { Send("error", new { text = "Ник должен быть в формате Имя_Фамилия латиницей (например, Ivan_Petrov)." }); return; }
        _busy = true;
        _cts = new CancellationTokenSource();
        bool showLoading = thenPlay && _cfg.ShowLoadingScreen;
        if (showLoading) Stage("check", "Проверка файлов игры…", 0.05);
        try
        {
            if (Gta.IsGameRunning())
            {
                if (thenPlay) { Send("loadingError", new { text = "GTA San Andreas уже запущена. Закройте игру и попробуйте снова." }); return; }
                Send("error", new { text = "Закройте GTA San Andreas перед установкой файлов." }); return;
            }
            if (_upd.Current == null) await _upd.LoadManifestAsync();
            if (_upd.Current == null)
            {
                if (!File.Exists(Path.Combine(_cfg.GtaPath, "cef.asi")))
                {
                    var msg = "Не удалось получить список файлов. Проверьте интернет или положите папку client рядом с лаунчером.";
                    if (showLoading) Send("loadingError", new { text = msg }); else Send("error", new { text = msg });
                    return;
                }
            }
            else if (_upd.Pending(_cfg.GtaPath, _cfg).Count > 0 || _upd.ToRemove(_cfg.GtaPath, _cfg).Count > 0)
            {
                long need = _upd.Pending(_cfg.GtaPath, _cfg).Sum(p => p.Size + p.Unpacked) / 1048576;
                long free = Gta.FreeSpaceMb(_cfg.GtaPath);
                if (free >= 0 && free < need + 50) throw new IOException($"Недостаточно места на диске: нужно ~{need + 50} МБ, свободно {free} МБ.");
                var prog = new Progress<InstallProgress>(p =>
                {
                    Send("progress", p);
                    if (showLoading) Stage("update", p.Text, 0.10 + p.Pct * 0.60, new { p.Speed, p.Eta });
                });
                await _upd.InstallAsync(_cfg.GtaPath, _cfg, prog, _cts.Token);
            }
            Send("state", State());
            if (!thenPlay) { Send("toast", new { text = "Сборка установлена ✔", ok = true }); return; }

            if (showLoading) Stage("prepare", "Подготовка профиля игрока…", 0.75);
            Gta.SetNick(_cfg.Nick);
            _cfg.RememberNick(_cfg.Nick); _cfg.Save();
            await Task.Delay(400);

            var (ip, port) = ServerAddr();
            if (showLoading) Stage("connect", $"Соединение с сервером {ip}:{port}…", 0.85);
            var info = await SampQuery.QueryAsync(ip, port);
            if (showLoading) Stage("connect", info.Online ? $"Сервер онлайн · {info.Players}/{info.MaxPlayers} игроков · пинг {info.Ping} мс" : "Сервер не ответил — пробуем подключиться напрямую…", 0.9);
            await Task.Delay(500);

            if (showLoading) Stage("launch", "Запуск GTA San Andreas…", 0.95);
            Gta.Launch(_cfg.GtaPath, ip, port);
            Log.Write($"launch {ip}:{port} as {_cfg.Nick}");
            for (int i = 0; i < 40 && !Gta.IsGameRunning(); i++) await Task.Delay(500);
            if (showLoading) Stage("done", Gta.IsGameRunning() ? "Игра запущена. Приятной игры!" : "Игра запускается…", 1.0);
            Send("launched", new { });
            if (_cfg.CloseOnPlay) { await Task.Delay(2500); Close(); }
        }
        catch (OperationCanceledException)
        {
            Send("loadingError", new { text = "Загрузка отменена.", cancelled = true });
        }
        catch (UnauthorizedAccessException)
        {
            var msg = "Нет доступа к папке игры. Запустите лаунчер от имени администратора или перенесите GTA из Program Files.";
            if (showLoading) Send("loadingError", new { text = msg }); else Send("error", new { text = msg });
        }
        catch (Exception ex)
        {
            Log.Write("install: " + ex);
            if (showLoading) Send("loadingError", new { text = ex.Message }); else Send("error", new { text = ex.Message });
        }
        finally
        {
            _busy = false; _cts?.Dispose(); _cts = null;
            Send("progress", new InstallProgress("", -1, 0, 0, "idle"));
            Send("state", State());
        }
    }

    private async Task LoadGamePackAsync()
    {
        if (_upd.Current == null) await _upd.LoadManifestAsync();
        var url = _upd.Current?.GameManifestUrl is { Length: > 0 } u ? u : _set.GameManifestUrl is { Length: > 0 } s2 ? s2 : LauncherSettings.DefaultGameUrl;
        Log.Write("game pack url: " + url);
        await _game.LoadAsync(url);
        Send("state", State());
    }

    /// <summary>Полная установка: скачивает GTA SA + SA-MP целиком, затем ставит всю сборку Godjo.</summary>
    private async Task DownloadGame()
    {
        if (_busy) return;
        if (_game.Pack == null) await LoadGamePackAsync();
        if (_game.Pack == null) { Send("error", new { text = "Сборка игры сейчас недоступна. Проверьте интернет или укажите папку с уже установленной GTA." }); return; }
        // как у крупных RP-проектов: игра ставится в папку лаунчера, без выбора пути
        string target = Path.Combine(AppContext.BaseDirectory, "game");
        if (target.Contains(@"\Program Files", StringComparison.OrdinalIgnoreCase))
        { Send("error", new { text = "Не устанавливайте игру в Program Files — выберите, например, C:\\Games." }); return; }
        if (File.Exists(Path.Combine(target, "gta_sa.exe")))
        { _cfg.GtaPath = target; _cfg.Save(); Send("toast", new { text = "В этой папке игра уже есть — используем её.", ok = true }); Send("state", State()); return; }

        _busy = true; _cts = new CancellationTokenSource();
        Stage("check", "Подготовка к загрузке игры…", 0.02);
        try
        {
            var prog = new Progress<InstallProgress>(p =>
            {
                Send("progress", p);
                Stage(p.Stage == "extract" ? "prepare" : "update", p.Text, 0.03 + p.Pct * 0.8, new { p.Speed, p.Eta });
            });
            await _game.InstallAsync(target, prog, _cts.Token);
            _cfg.GtaPath = target; _cfg.Save();
            Log.Write("game ready: " + target);
            if (_upd.Current == null) await _upd.LoadManifestAsync();
            if (_upd.Current != null)
            {
                Stage("prepare", "Установка сборки Godjo (интерфейс, машины, текстуры)…", 0.85);
                var prog2 = new Progress<InstallProgress>(p => Stage("prepare", p.Text, 0.85 + p.Pct * 0.14, new { p.Speed, p.Eta }));
                await _upd.InstallAsync(target, _cfg, prog2, _cts.Token);
            }
            Stage("done", "Игра и сборка Godjo установлены! Введите ник и нажмите «Играть».", 1.0);
        }
        catch (OperationCanceledException) { Send("loadingError", new { text = "Загрузка приостановлена. Нажмите «Скачать игру» ещё раз — она продолжится с того же места.", cancelled = true }); }
        catch (Exception ex) { Log.Write("game: " + ex); Send("loadingError", new { text = ex.Message }); }
        finally
        {
            _busy = false; _cts?.Dispose(); _cts = null;
            Send("progress", new InstallProgress("", -1, 0, 0, "idle"));
            Send("state", State());
        }
    }

    private async Task QueryLoop()
    {
        while (!IsDisposed)
        {
            var (ip, port) = ServerAddr();
            var info = await SampQuery.QueryAsync(ip, port);
            _lastInfo = info;
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
