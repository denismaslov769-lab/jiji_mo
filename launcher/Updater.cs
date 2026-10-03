using System.Diagnostics;
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
    [JsonPropertyName("tips")] public List<string> Tips { get; set; } = new();
    [JsonPropertyName("gameManifestUrl")] public string GameManifestUrl { get; set; } = ""; // полная сборка игры (game.json)
}
public sealed class ManifestServer { [JsonPropertyName("name")] public string Name { get; set; } = ""; [JsonPropertyName("ip")] public string Ip { get; set; } = ""; [JsonPropertyName("port")] public int Port { get; set; } }
public sealed class NewsItem { [JsonPropertyName("title")] public string Title { get; set; } = ""; [JsonPropertyName("text")] public string Text { get; set; } = ""; [JsonPropertyName("date")] public string Date { get; set; } = ""; [JsonPropertyName("tag")] public string Tag { get; set; } = ""; }
public sealed class LauncherInfo { [JsonPropertyName("version")] public string Version { get; set; } = ""; [JsonPropertyName("url")] public string Url { get; set; } = ""; }
public sealed class Package
{
    [JsonPropertyName("name")] public string Name { get; set; } = "";
    [JsonPropertyName("title")] public string Title { get; set; } = "";
    [JsonPropertyName("description")] public string Description { get; set; } = "";
    [JsonPropertyName("category")] public string Category { get; set; } = "core";      // core | build | optional
    [JsonPropertyName("url")] public string Url { get; set; } = "";
    [JsonPropertyName("sha256")] public string Sha256 { get; set; } = "";
    [JsonPropertyName("size")] public long Size { get; set; }
    [JsonPropertyName("unpacked")] public long Unpacked { get; set; }
    [JsonPropertyName("check")] public string Check { get; set; } = "";              // файл, наличие которого проверяется
    [JsonPropertyName("skipIfExists")] public string SkipIfExists { get; set; } = "";  // не ставить, если файл уже есть (ASI loader)
    [JsonPropertyName("optional")] public bool Optional { get; set; }                  // можно выключить в «Сборке»
    [JsonPropertyName("default")] public bool Default { get; set; } = true;            // включён по умолчанию
    [JsonPropertyName("requires")] public string Requires { get; set; } = "";          // имя пакета-зависимости
    [JsonPropertyName("removeDir")] public string RemoveDir { get; set; } = "";        // папка, удаляемая при выключении мода
}

public sealed record PackageView(string Name, string Title, string Description, string Category, long Size, bool Optional, bool Enabled, bool Installed, bool UpToDate);

/// <summary>Состояние установленных пакетов: godjo\installed.json (name → sha) + списки файлов godjo\files\*.txt.</summary>
public sealed class Updater
{
    private static readonly HttpClient Http = new() { Timeout = TimeSpan.FromMinutes(60) };
    static Updater() { Http.DefaultRequestHeaders.UserAgent.ParseAdd("GodjoLauncher/2.0"); }

    private readonly LauncherSettings _settings;
    public Manifest? Current { get; private set; }
    private string? _localDir;

    public Updater(LauncherSettings s) { _settings = s; }

    /// <summary>Сначала интернет, затем пакет рядом с лаунчером (client\manifest.json).</summary>
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
                var lm = JsonSerializer.Deserialize<Manifest>(await File.ReadAllTextAsync(local));
                if (lm?.Version == Current.Version) _localDir = Path.GetDirectoryName(local);
            }
            return Current;
        }
        catch (Exception ex) { Log.Write("manifest online: " + ex.Message); }
        if (File.Exists(local))
        {
            try
            {
                Current = JsonSerializer.Deserialize<Manifest>(await File.ReadAllTextAsync(local));
                _localDir = Path.GetDirectoryName(local);
            }
            catch (Exception ex) { Log.Write("manifest local: " + ex.Message); }
        }
        return Current;
    }

    private static string StatePath(string gta) => Path.Combine(gta, "godjo", "installed.json");
    private static string FilesPath(string gta, string pkg) => Path.Combine(gta, "godjo", "files", pkg + ".txt");

    private static Dictionary<string, string> LoadState(string gta)
    {
        try { return JsonSerializer.Deserialize<Dictionary<string, string>>(File.ReadAllText(StatePath(gta))) ?? new(); }
        catch { return new(); }
    }
    private static void SaveState(string gta, Dictionary<string, string> st)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(StatePath(gta))!);
        File.WriteAllText(StatePath(gta), JsonSerializer.Serialize(st));
    }

    public static bool IsEnabled(Package p, Config cfg) => !p.Optional || (cfg.Mods.TryGetValue(p.Name, out var on) ? on : p.Default);

    private bool Skipped(Package p, string gta, Dictionary<string, string> st) =>
        !string.IsNullOrEmpty(p.SkipIfExists) && !st.ContainsKey(p.Name) &&
        p.SkipIfExists.Split(';', StringSplitOptions.RemoveEmptyEntries).Any(f => File.Exists(Path.Combine(gta, f.Trim())));

    private bool NeedsInstall(Package p, string gta, Dictionary<string, string> st)
    {
        if (Skipped(p, gta, st)) return false;
        bool missing = !string.IsNullOrEmpty(p.Check) && !File.Exists(Path.Combine(gta, p.Check));
        return missing || !st.TryGetValue(p.Name, out var sha) || !string.Equals(sha, p.Sha256, StringComparison.OrdinalIgnoreCase);
    }

    /// <summary>Пакеты, которые нужно скачать/обновить с учётом включённых модов.</summary>
    public List<Package> Pending(string gta, Config cfg)
    {
        var res = new List<Package>();
        if (Current == null) return res;
        var st = LoadState(gta);
        foreach (var p in Current.Packages)
        {
            if (!IsEnabled(p, cfg)) continue;
            if (!string.IsNullOrEmpty(p.Requires) && Current.Packages.FirstOrDefault(x => x.Name == p.Requires) is { } dep && !IsEnabled(dep, cfg)) continue;
            if (NeedsInstall(p, gta, st)) res.Add(p);
        }
        return res;
    }

    /// <summary>Установленные моды, которые пользователь выключил — их надо удалить.</summary>
    public List<Package> ToRemove(string gta, Config cfg)
    {
        if (Current == null) return new();
        var st = LoadState(gta);
        return Current.Packages.Where(p => p.Optional && !IsEnabled(p, cfg) && st.ContainsKey(p.Name)).ToList();
    }

    public List<PackageView> Views(string gta, Config cfg)
    {
        if (Current == null) return new();
        var st = LoadState(gta);
        bool hasGta = File.Exists(Path.Combine(gta ?? "", "gta_sa.exe"));
        return Current.Packages.Select(p =>
        {
            bool installed = hasGta && (st.ContainsKey(p.Name) || Skipped(p, gta!, st));
            bool fresh = hasGta && !NeedsInstall(p, gta!, st);
            return new PackageView(p.Name, string.IsNullOrEmpty(p.Title) ? p.Name : p.Title, p.Description, p.Category, p.Size, p.Optional, IsEnabled(p, cfg), installed, fresh);
        }).ToList();
    }

    public async Task InstallAsync(string gta, Config cfg, IProgress<InstallProgress> progress, CancellationToken ct = default)
    {
        foreach (var r in ToRemove(gta, cfg)) Remove(gta, r);
        var list = Pending(gta, cfg);
        var st = LoadState(gta);
        var tmpDir = Path.Combine(Path.GetTempPath(), "GodjoLauncher");
        Directory.CreateDirectory(tmpDir);
        long totalBytes = list.Sum(p => Math.Max(p.Size, 1)), doneBefore = 0;
        int i = 0;
        foreach (var p in list)
        {
            ct.ThrowIfCancellationRequested();
            i++;
            string label = $"[{i}/{list.Count}] {(string.IsNullOrEmpty(p.Title) ? p.Name : p.Title)}";
            string zip;
            var localFile = _localDir != null ? Path.Combine(_localDir, Path.GetFileName(p.Url.Split('?')[0].Replace('\\', '/'))) : null;
            if (localFile != null && File.Exists(localFile)) zip = localFile;
            else
            {
                zip = Path.Combine(tmpDir, p.Name + ".zip");
                var url = Uri.IsWellFormedUriString(p.Url, UriKind.Absolute) ? p.Url : new Uri(new Uri(_settings.ManifestUrl), p.Url).ToString();
                Log.Write("download " + url);
                using var resp = await Http.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, ct);
                resp.EnsureSuccessStatusCode();
                long total = resp.Content.Headers.ContentLength ?? p.Size;
                await using (var src = await resp.Content.ReadAsStreamAsync(ct))
                await using (var dst = File.Create(zip))
                {
                    var buf = new byte[1 << 16];
                    long done = 0; int n; var last = DateTime.MinValue;
                    var sw = Stopwatch.StartNew();
                    while ((n = await src.ReadAsync(buf, ct)) > 0)
                    {
                        await dst.WriteAsync(buf.AsMemory(0, n), ct);
                        done += n;
                        if ((DateTime.Now - last).TotalMilliseconds > 150)
                        {
                            last = DateTime.Now;
                            double speed = done / Math.Max(sw.Elapsed.TotalSeconds, 0.001);
                            double eta = speed > 0 && total > done ? (total - done) / speed : 0;
                            progress.Report(new InstallProgress($"{label}: загрузка {done / 1048576.0:0.0} / {total / 1048576.0:0.0} МБ",
                                (double)(doneBefore + done) / totalBytes, speed, eta, "download"));
                        }
                    }
                }
            }
            progress.Report(new InstallProgress($"{label}: проверка целостности", (double)(doneBefore + p.Size) / totalBytes, 0, 0, "verify"));
            if (!string.IsNullOrEmpty(p.Sha256))
            {
                await using var fs = File.OpenRead(zip);
                var hash = Convert.ToHexString(await SHA256.HashDataAsync(fs, ct));
                if (!hash.Equals(p.Sha256, StringComparison.OrdinalIgnoreCase))
                {
                    if (zip.StartsWith(tmpDir)) try { File.Delete(zip); } catch { }
                    throw new InvalidDataException($"Файл {p.Name} повреждён (хэш не совпадает). Попробуйте ещё раз.");
                }
            }
            progress.Report(new InstallProgress($"{label}: установка", (double)(doneBefore + p.Size) / totalBytes, 0, 0, "extract"));
            var files = await Task.Run(() => Extract(zip, gta), ct);
            Directory.CreateDirectory(Path.GetDirectoryName(FilesPath(gta, p.Name))!);
            await File.WriteAllLinesAsync(FilesPath(gta, p.Name), files, ct);
            st[p.Name] = p.Sha256;
            SaveState(gta, st);
            if (zip.StartsWith(tmpDir)) try { File.Delete(zip); } catch { }
            doneBefore += Math.Max(p.Size, 1);
            Log.Write("installed " + p.Name);
        }
        progress.Report(new InstallProgress("Все файлы установлены", 1, 0, 0, "done"));
    }

    /// <summary>Удаляет файлы выключенного мода.</summary>
    public void Remove(string gta, Package p)
    {
        var root = Path.GetFullPath(gta);
        var fp = FilesPath(gta, p.Name);
        if (File.Exists(fp))
        {
            foreach (var line in File.ReadAllLines(fp))
            {
                var rel = line.Split('|')[0];
                if (string.IsNullOrWhiteSpace(rel)) continue;
                var full = Path.GetFullPath(Path.Combine(root, rel));
                if (!full.StartsWith(root, StringComparison.OrdinalIgnoreCase)) continue;
                try { if (File.Exists(full)) File.Delete(full); } catch (Exception ex) { Log.Write("remove " + rel + ": " + ex.Message); }
            }
            File.Delete(fp);
        }
        if (!string.IsNullOrEmpty(p.RemoveDir))
        {
            var dir = Path.GetFullPath(Path.Combine(root, p.RemoveDir));
            if (dir.StartsWith(root, StringComparison.OrdinalIgnoreCase) && dir.Length > root.Length + 3 && Directory.Exists(dir))
                try { Directory.Delete(dir, true); } catch (Exception ex) { Log.Write("remove dir: " + ex.Message); }
        }
        var st = LoadState(gta);
        if (st.Remove(p.Name)) SaveState(gta, st);
        Log.Write("removed " + p.Name);
    }

    /// <summary>
    /// Проверка целостности установленных пакетов. Быстрая (при каждом «Играть»): наличие и размер каждого файла.
    /// Полная (deep, кнопка «Проверить файлы»): ещё и CRC32 содержимого. Повреждённые пакеты помечаются к переустановке.
    /// </summary>
    public int Verify(string gta, bool deep = false, CancellationToken ct = default)
    {
        var st = LoadState(gta);
        int broken = 0;
        foreach (var name in st.Keys.ToList())
        {
            ct.ThrowIfCancellationRequested();
            var fp = FilesPath(gta, name);
            bool bad = !File.Exists(fp);
            if (!bad)
                foreach (var line in File.ReadAllLines(fp))
                {
                    var f = line.Split('|');
                    if (string.IsNullOrWhiteSpace(f[0])) continue;
                    var full = Path.Combine(gta, f[0]);
                    if (!File.Exists(full)) { bad = true; Log.Write($"verify {name}: нет {f[0]}"); break; }
                    if (f.Length >= 3 && long.TryParse(f[1], out var size) && new FileInfo(full).Length != size) { bad = true; Log.Write($"verify {name}: размер {f[0]}"); break; }
                    if (deep && f.Length >= 3 && uint.TryParse(f[2], System.Globalization.NumberStyles.HexNumber, null, out var crc) &&
                        FileHashCache.ComputeAsync(full, ct).GetAwaiter().GetResult() != crc) { bad = true; Log.Write($"verify {name}: изменён {f[0]}"); break; }
                }
            if (bad) { st.Remove(name); broken++; }
        }
        SaveState(gta, st);
        return broken;
    }

    /// <summary>Сбросить отметки установки — всё будет скачано заново.</summary>
    public static void ResetState(string gta)
    {
        try { if (File.Exists(StatePath(gta))) File.Delete(StatePath(gta)); } catch { }
    }

    private static List<string> Extract(string zip, string dest)
    {
        var root = Path.GetFullPath(dest);
        var files = new List<string>();
        using var za = ZipFile.OpenRead(zip);
        foreach (var e in za.Entries)
        {
            var target = Path.GetFullPath(Path.Combine(root, e.FullName));
            if (!target.StartsWith(root, StringComparison.OrdinalIgnoreCase)) continue; // защита от zip-slip
            if (string.IsNullOrEmpty(e.Name)) { Directory.CreateDirectory(target); continue; }
            Directory.CreateDirectory(Path.GetDirectoryName(target)!);
            try { e.ExtractToFile(target, true); }
            catch (IOException) { throw new IOException($"Не удалось записать {e.FullName}. Закройте игру и запустите лаунчер от имени администратора, если GTA в Program Files."); }
            files.Add($"{e.FullName.Replace('/', '\\')}|{e.Length}|{e.Crc32:X8}");
        }
        return files;
    }
}

public sealed record InstallProgress(string Text, double Pct, double Speed, double Eta, string Stage);
