using System.IO.Compression;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace GodjoLauncher;

public sealed class Manifest
{
    [JsonPropertyName("version")] public string Version { get; set; } = "";
    [JsonPropertyName("server")] public ManifestServer? Server { get; set; }
    [JsonPropertyName("news")] public List<NewsItem> News { get; set; } = new();
    [JsonPropertyName("packages")] public List<Package> Packages { get; set; } = new();
    [JsonPropertyName("launcher")] public LauncherInfo? Launcher { get; set; }
}
public sealed class ManifestServer { [JsonPropertyName("name")] public string Name { get; set; } = ""; [JsonPropertyName("ip")] public string Ip { get; set; } = ""; [JsonPropertyName("port")] public int Port { get; set; } }
public sealed class NewsItem { [JsonPropertyName("title")] public string Title { get; set; } = ""; [JsonPropertyName("text")] public string Text { get; set; } = ""; [JsonPropertyName("date")] public string Date { get; set; } = ""; }
public sealed class LauncherInfo { [JsonPropertyName("version")] public string Version { get; set; } = ""; [JsonPropertyName("url")] public string Url { get; set; } = ""; }
public sealed class Package
{
    [JsonPropertyName("name")] public string Name { get; set; } = "";
    [JsonPropertyName("title")] public string Title { get; set; } = "";
    [JsonPropertyName("url")] public string Url { get; set; } = "";
    [JsonPropertyName("sha256")] public string Sha256 { get; set; } = "";
    [JsonPropertyName("size")] public long Size { get; set; }
    [JsonPropertyName("check")] public string Check { get; set; } = "";              // файл, наличие которого проверяется
    [JsonPropertyName("skipIfExists")] public string SkipIfExists { get; set; } = "";  // не ставить, если файл уже есть (ASI loader)
}

public sealed class Updater
{
    private static readonly HttpClient Http = new() { Timeout = TimeSpan.FromMinutes(30) };
    static Updater() { Http.DefaultRequestHeaders.UserAgent.ParseAdd("GodjoLauncher/1.0"); }

    private readonly LauncherSettings _settings;
    public Manifest? Current { get; private set; }
    private string? _localDir;

    public Updater(LauncherSettings s) { _settings = s; }

    /// <summary>Сначала ищем пакет рядом с лаунчером (client\manifest.json), затем — в интернете.</summary>
    public async Task<Manifest?> LoadManifestAsync()
    {
        var local = Path.Combine(AppContext.BaseDirectory, "client", "manifest.json");
        try
        {
            var json = await Http.GetStringAsync(_settings.ManifestUrl);
            Current = JsonSerializer.Deserialize<Manifest>(json);
            _localDir = null;
            if (Current != null && File.Exists(local))
            {
                // если локальная версия совпадает — берём файлы локально (быстрее)
                var lm = JsonSerializer.Deserialize<Manifest>(await File.ReadAllTextAsync(local));
                if (lm?.Version == Current.Version) _localDir = Path.GetDirectoryName(local);
            }
            return Current;
        }
        catch { }
        if (File.Exists(local))
        {
            Current = JsonSerializer.Deserialize<Manifest>(await File.ReadAllTextAsync(local));
            _localDir = Path.GetDirectoryName(local);
        }
        return Current;
    }

    private static string StatePath(string gta) => Path.Combine(gta, "godjo", "installed.json");

    private static Dictionary<string, string> LoadState(string gta)
    {
        try { return JsonSerializer.Deserialize<Dictionary<string, string>>(File.ReadAllText(StatePath(gta))) ?? new(); }
        catch { return new(); }
    }

    public List<Package> Pending(string gta)
    {
        var res = new List<Package>();
        if (Current == null) return res;
        var st = LoadState(gta);
        foreach (var p in Current.Packages)
        {
            if (!string.IsNullOrEmpty(p.SkipIfExists) && !st.ContainsKey(p.Name) && p.SkipIfExists.Split(';', StringSplitOptions.RemoveEmptyEntries).Any(f => File.Exists(Path.Combine(gta, f.Trim())))) continue;
            bool missing = !string.IsNullOrEmpty(p.Check) && !File.Exists(Path.Combine(gta, p.Check));
            if (missing || !st.TryGetValue(p.Name, out var sha) || !string.Equals(sha, p.Sha256, StringComparison.OrdinalIgnoreCase)) res.Add(p);
        }
        return res;
    }

    public async Task InstallAsync(string gta, IProgress<(string text, double pct)> progress, CancellationToken ct = default)
    {
        var list = Pending(gta);
        var st = LoadState(gta);
        var tmpDir = Path.Combine(Path.GetTempPath(), "GodjoLauncher");
        Directory.CreateDirectory(tmpDir);
        int i = 0;
        foreach (var p in list)
        {
            i++;
            string label = $"[{i}/{list.Count}] {(string.IsNullOrEmpty(p.Title) ? p.Name : p.Title)}";
            string zip;
            var localFile = _localDir != null ? Path.Combine(_localDir, Path.GetFileName(p.Url.Split('?')[0].Replace('\\', '/'))) : null;
            if (localFile != null && File.Exists(localFile)) zip = localFile;
            else
            {
                zip = Path.Combine(tmpDir, p.Name + ".zip");
                var url = Uri.IsWellFormedUriString(p.Url, UriKind.Absolute) ? p.Url : new Uri(new Uri(_settings.ManifestUrl), p.Url).ToString();
                using var resp = await Http.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, ct);
                resp.EnsureSuccessStatusCode();
                long total = resp.Content.Headers.ContentLength ?? p.Size;
                await using var src = await resp.Content.ReadAsStreamAsync(ct);
                await using var dst = File.Create(zip);
                var buf = new byte[1 << 16];
                long done = 0; int n; var last = DateTime.MinValue;
                while ((n = await src.ReadAsync(buf, ct)) > 0)
                {
                    await dst.WriteAsync(buf.AsMemory(0, n), ct);
                    done += n;
                    if ((DateTime.Now - last).TotalMilliseconds > 150)
                    {
                        last = DateTime.Now;
                        progress.Report(($"{label}: загрузка {done / 1048576.0:0.0} / {total / 1048576.0:0.0} МБ", total > 0 ? (double)done / total : 0));
                    }
                }
            }
            progress.Report(($"{label}: проверка целостности", 1));
            if (!string.IsNullOrEmpty(p.Sha256))
            {
                await using var fs = File.OpenRead(zip);
                var hash = Convert.ToHexString(await SHA256.HashDataAsync(fs, ct));
                if (!hash.Equals(p.Sha256, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException($"Файл {p.Name} повреждён (хэш не совпадает). Попробуйте ещё раз.");
            }
            progress.Report(($"{label}: установка", 1));
            await Task.Run(() => Extract(zip, gta), ct);
            st[p.Name] = p.Sha256;
            Directory.CreateDirectory(Path.GetDirectoryName(StatePath(gta))!);
            await File.WriteAllTextAsync(StatePath(gta), JsonSerializer.Serialize(st), ct);
        }
        progress.Report(("Все файлы установлены", 1));
    }

    private static void Extract(string zip, string dest)
    {
        var root = Path.GetFullPath(dest);
        using var za = ZipFile.OpenRead(zip);
        foreach (var e in za.Entries)
        {
            var target = Path.GetFullPath(Path.Combine(root, e.FullName));
            if (!target.StartsWith(root, StringComparison.OrdinalIgnoreCase)) continue; // защита от zip-slip
            if (string.IsNullOrEmpty(e.Name)) { Directory.CreateDirectory(target); continue; }
            Directory.CreateDirectory(Path.GetDirectoryName(target)!);
            try { e.ExtractToFile(target, true); }
            catch (IOException) { throw new IOException($"Не удалось записать {e.FullName}. Закройте игру и запустите лаунчер от имени администратора, если GTA в Program Files."); }
        }
    }
}
