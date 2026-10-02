using System.Diagnostics;
using System.IO.Compression;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.Win32;

namespace GodjoLauncher;

/// <summary>Описание полной сборки игры (game.json рядом с частями архива на хостинге).</summary>
public sealed class GamePack
{
    [JsonPropertyName("version")] public string Version { get; set; } = "";
    [JsonPropertyName("title")] public string Title { get; set; } = "GTA San Andreas — сборка Godjo RP";
    [JsonPropertyName("size")] public long Size { get; set; }          // сумма частей
    [JsonPropertyName("unpacked")] public long Unpacked { get; set; }  // размер после распаковки
    [JsonPropertyName("files")] public int Files { get; set; }
    [JsonPropertyName("parts")] public List<GamePart> Parts { get; set; } = new();
}
public sealed class GamePart
{
    [JsonPropertyName("url")] public string Url { get; set; } = "";
    [JsonPropertyName("size")] public long Size { get; set; }
    [JsonPropertyName("sha256")] public string Sha256 { get; set; } = "";
}

/// <summary>
/// Скачивание и распаковка полной сборки GTA SA + SA-MP (как у крупных RP-проектов).
/// Архив режется на части (любой хостинг), докачка через HTTP Range, проверка SHA-256 каждой части,
/// распаковка прямо из частей без склейки (экономит место на диске).
/// </summary>
public sealed class GameDownloader
{
    private static readonly HttpClient Http = new() { Timeout = Timeout.InfiniteTimeSpan };
    static GameDownloader() { Http.DefaultRequestHeaders.UserAgent.ParseAdd("GodjoLauncher/2.0"); }

    public GamePack? Pack { get; private set; }
    public string PackUrl { get; private set; } = "";

    public async Task<GamePack?> LoadAsync(string url)
    {
        PackUrl = url;
        if (string.IsNullOrWhiteSpace(url)) return Pack = null;
        try
        {
            var json = await Http.GetStringAsync(url);
            Pack = JsonSerializer.Deserialize<GamePack>(json);
            if (Pack != null && Pack.Parts.Count == 0) Pack = null;
        }
        catch (Exception ex) { Log.Write("game.json: " + ex.Message); Pack = null; }
        return Pack;
    }

    private string PartUrl(GamePart p) =>
        Uri.IsWellFormedUriString(p.Url, UriKind.Absolute) ? p.Url : new Uri(new Uri(PackUrl), p.Url).ToString();

    public static long FreeBytes(string dir)
    {
        try { return new DriveInfo(Path.GetPathRoot(Path.GetFullPath(dir))!).AvailableFreeSpace; } catch { return long.MaxValue; }
    }

    public async Task InstallAsync(string target, IProgress<InstallProgress> progress, CancellationToken ct)
    {
        if (Pack == null) throw new InvalidOperationException("Сборка игры недоступна.");
        target = Path.GetFullPath(target);
        Directory.CreateDirectory(target);
        var tmp = Path.Combine(target, "_godjo_download");
        Directory.CreateDirectory(tmp);

        long have = Pack.Parts.Select((p, i) => File.Exists(PartPath(tmp, i)) ? new FileInfo(PartPath(tmp, i)).Length : 0).Sum();
        long need = Pack.Size - have + Pack.Unpacked + 200L * 1048576;
        long free = FreeBytes(target);
        if (free < need)
            throw new IOException($"Недостаточно места на диске: нужно ~{need / 1073741824.0:0.0} ГБ, свободно {free / 1073741824.0:0.0} ГБ.");

        // ---------- 1. загрузка частей ----------
        long total = Math.Max(1, Pack.Size), doneBefore = 0;
        var sw = Stopwatch.StartNew(); long sessionBytes = 0;
        for (int i = 0; i < Pack.Parts.Count; i++)
        {
            ct.ThrowIfCancellationRequested();
            var part = Pack.Parts[i];
            var path = PartPath(tmp, i);
            var okMark = path + ".ok";
            string label = $"Загрузка игры: часть {i + 1} из {Pack.Parts.Count}";
            if (File.Exists(okMark) && File.Exists(path) && new FileInfo(path).Length == part.Size) { doneBefore += part.Size; continue; }

            for (int attempt = 1; ; attempt++)
            {
                long existing = File.Exists(path) ? new FileInfo(path).Length : 0;
                if (existing > part.Size) { File.Delete(path); existing = 0; }
                if (existing < part.Size)
                {
                    using var req = new HttpRequestMessage(HttpMethod.Get, PartUrl(part));
                    if (existing > 0) req.Headers.Range = new RangeHeaderValue(existing, null);
                    using var resp = await Http.SendAsync(req, HttpCompletionOption.ResponseHeadersRead, ct);
                    resp.EnsureSuccessStatusCode();
                    bool resumed = existing > 0 && resp.StatusCode == System.Net.HttpStatusCode.PartialContent;
                    if (!resumed) existing = 0;
                    await using var src = await resp.Content.ReadAsStreamAsync(ct);
                    await using var dst = new FileStream(path, resumed ? FileMode.Append : FileMode.Create, FileAccess.Write, FileShare.None, 1 << 20);
                    var buf = new byte[1 << 20]; int n; long done = existing; var last = DateTime.MinValue;
                    while ((n = await src.ReadAsync(buf, ct)) > 0)
                    {
                        await dst.WriteAsync(buf.AsMemory(0, n), ct);
                        done += n; sessionBytes += n;
                        if ((DateTime.Now - last).TotalMilliseconds > 200)
                        {
                            last = DateTime.Now;
                            double speed = sessionBytes / Math.Max(sw.Elapsed.TotalSeconds, 0.001);
                            long left = total - doneBefore - done;
                            progress.Report(new InstallProgress($"{label} · {(doneBefore + done) / 1073741824.0:0.00} / {total / 1073741824.0:0.00} ГБ",
                                0.85 * (doneBefore + done) / total, speed, speed > 0 ? left / speed : 0, "download"));
                        }
                    }
                }
                progress.Report(new InstallProgress($"Проверка части {i + 1} из {Pack.Parts.Count}…", 0.85 * (doneBefore + part.Size) / total, 0, 0, "verify"));
                if (string.IsNullOrEmpty(part.Sha256) || await ShaOk(path, part.Sha256, ct)) break;
                File.Delete(path);
                if (attempt >= 3) throw new InvalidDataException($"Часть {i + 1} скачалась с ошибкой (хэш не совпадает). Попробуйте позже.");
                Log.Write($"part {i + 1} sha mismatch, retry {attempt}");
            }
            File.WriteAllText(okMark, part.Sha256);
            doneBefore += part.Size;
        }

        // ---------- 2. распаковка прямо из частей ----------
        progress.Report(new InstallProgress("Распаковка игры…", 0.86, 0, 0, "extract"));
        await Task.Run(() =>
        {
            using var cs = new ConcatStream(Enumerable.Range(0, Pack.Parts.Count).Select(i => PartPath(tmp, i)).ToList());
            using var zip = new ZipArchive(cs, ZipArchiveMode.Read);
            long all = Math.Max(1, zip.Entries.Sum(e => e.Length)), done = 0; int k = 0;
            var last = DateTime.MinValue;
            foreach (var e in zip.Entries)
            {
                ct.ThrowIfCancellationRequested();
                var dest = Path.GetFullPath(Path.Combine(target, e.FullName));
                if (!dest.StartsWith(target, StringComparison.OrdinalIgnoreCase)) continue;
                if (string.IsNullOrEmpty(e.Name)) { Directory.CreateDirectory(dest); continue; }
                Directory.CreateDirectory(Path.GetDirectoryName(dest)!);
                e.ExtractToFile(dest, true);
                done += e.Length; k++;
                if ((DateTime.Now - last).TotalMilliseconds > 200)
                {
                    last = DateTime.Now;
                    progress.Report(new InstallProgress($"Распаковка игры: {k} / {zip.Entries.Count} файлов", 0.86 + 0.13 * done / all, 0, 0, "extract"));
                }
            }
        }, ct);

        // ---------- 3. регистрация для SA-MP и уборка ----------
        try
        {
            using var key = Registry.CurrentUser.CreateSubKey(@"Software\SAMP");
            key.SetValue("gta_sa_exe", Path.Combine(target, "gta_sa.exe"));
        }
        catch { }
        try { Directory.Delete(tmp, true); } catch { }
        Log.Write("game installed to " + target);
        progress.Report(new InstallProgress("Игра установлена", 1, 0, 0, "done"));
    }

    private static string PartPath(string tmp, int i) => Path.Combine(tmp, $"game.zip.{i + 1:000}");

    private static async Task<bool> ShaOk(string path, string sha, CancellationToken ct)
    {
        await using var fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 20);
        return Convert.ToHexString(await SHA256.HashDataAsync(fs, ct)).Equals(sha, StringComparison.OrdinalIgnoreCase);
    }

    // =====================================================================
    //  Создание сборки: GodjoLauncher.exe --make-game-pack "<папка GTA>" "<куда>" [размер части, МБ]
    // =====================================================================
    private static readonly string[] SkipDirs = { "_godjo_download", "godjo", @"modloader\godjo_cars", @"modloader\godjo_textures", @"cef\assets\godjo", "screens", "SAMP\\screens" };
    private static readonly string[] SkipExt = { ".log", ".tmp", ".bak" };

    public static string MakePack(string gta, string outDir, int partMb, Action<string>? report = null)
    {
        gta = Path.GetFullPath(gta.TrimEnd('\\', '/'));
        if (!File.Exists(Path.Combine(gta, "gta_sa.exe"))) throw new FileNotFoundException("В папке нет gta_sa.exe: " + gta);
        if (!File.Exists(Path.Combine(gta, "samp.dll"))) throw new FileNotFoundException("В папке нет samp.dll — установите SA-MP 0.3.7-R1/R3 в эту копию игры.");
        Directory.CreateDirectory(outDir);
        var zipPath = Path.Combine(outDir, "game.zip");
        var files = Directory.EnumerateFiles(gta, "*", SearchOption.AllDirectories)
            .Where(f =>
            {
                var rel = Path.GetRelativePath(gta, f);
                if (SkipDirs.Any(d => rel.StartsWith(d + "\\", StringComparison.OrdinalIgnoreCase))) return false;
                if (SkipExt.Contains(Path.GetExtension(f).ToLowerInvariant())) return false;
                // моды Godjo ставятся отдельными пакетами — в базовую сборку не кладём
                return !new[] { "cef.asi", "chatlog.txt" }.Contains(Path.GetFileName(f).ToLowerInvariant());
            }).ToList();
        long unpacked = 0; int n = 0;
        using (var fs = File.Create(zipPath))
        using (var zip = new ZipArchive(fs, ZipArchiveMode.Create))
        {
            foreach (var f in files)
            {
                var rel = Path.GetRelativePath(gta, f).Replace('\\', '/');
                var ext = Path.GetExtension(f).ToLowerInvariant();
                var level = ext is ".img" or ".dff" or ".txd" or ".dat" or ".ide" or ".ipl" or ".exe" or ".dll" or ".asi" or ".cfg" or ".txt" or ".scm"
                    ? CompressionLevel.Optimal : CompressionLevel.Fastest;
                zip.CreateEntryFromFile(f, rel, level);
                unpacked += new FileInfo(f).Length;
                if (++n % 50 == 0) report?.Invoke($"Упаковка: {n} / {files.Count}");
            }
        }
        // нарезка на части
        var pack = new GamePack { Version = DateTime.Now.ToString("yyyy.MM.dd"), Unpacked = unpacked, Files = files.Count };
        long partSize = (long)partMb * 1048576;
        using (var src = File.OpenRead(zipPath))
        {
            var buf = new byte[1 << 20];
            for (int i = 1; src.Position < src.Length; i++)
            {
                var name = $"game.zip.{i:000}";
                using var sha = SHA256.Create();
                long written = 0;
                using (var dst = File.Create(Path.Combine(outDir, name)))
                {
                    while (written < partSize)
                    {
                        int r = src.Read(buf, 0, (int)Math.Min(buf.Length, partSize - written));
                        if (r <= 0) break;
                        dst.Write(buf, 0, r); sha.TransformBlock(buf, 0, r, null, 0); written += r;
                    }
                }
                sha.TransformFinalBlock(Array.Empty<byte>(), 0, 0);
                pack.Parts.Add(new GamePart { Url = name, Size = written, Sha256 = Convert.ToHexString(sha.Hash!) });
                report?.Invoke($"Часть {i}: {written / 1048576} МБ");
            }
        }
        File.Delete(zipPath);
        pack.Size = pack.Parts.Sum(p => p.Size);
        File.WriteAllText(Path.Combine(outDir, "game.json"), JsonSerializer.Serialize(pack, new JsonSerializerOptions { WriteIndented = true }));
        return $"Готово: {files.Count} файлов, {unpacked / 1048576} МБ → {pack.Parts.Count} частей ({pack.Size / 1048576} МБ).\n" +
               $"Загрузите ВСЁ содержимое папки\n{outDir}\nна хостинг и укажите ссылку на game.json в launcher.json (gameManifestUrl).";
    }
}

/// <summary>Поток только для чтения, склеивающий несколько файлов (части архива) без копирования.</summary>
internal sealed class ConcatStream : Stream
{
    private readonly List<FileStream> _parts;
    private readonly long[] _starts;
    private long _pos;
    public ConcatStream(List<string> paths)
    {
        _parts = paths.Select(p => new FileStream(p, FileMode.Open, FileAccess.Read, FileShare.Read, 1 << 16)).ToList();
        _starts = new long[_parts.Count];
        long acc = 0;
        for (int i = 0; i < _parts.Count; i++) { _starts[i] = acc; acc += _parts[i].Length; }
        Length = acc;
    }
    public override long Length { get; }
    public override bool CanRead => true;
    public override bool CanSeek => true;
    public override bool CanWrite => false;
    public override long Position { get => _pos; set => _pos = value; }
    public override int Read(byte[] buffer, int offset, int count)
    {
        if (_pos >= Length) return 0;
        int idx = Array.BinarySearch(_starts, _pos);
        if (idx < 0) idx = ~idx - 1;
        while (idx < _parts.Count - 1 && _starts[idx + 1] <= _pos) idx++;
        var fs = _parts[idx];
        fs.Position = _pos - _starts[idx];
        int r = fs.Read(buffer, offset, (int)Math.Min(count, fs.Length - fs.Position));
        _pos += r;
        return r;
    }
    public override long Seek(long offset, SeekOrigin origin) =>
        _pos = origin switch { SeekOrigin.Begin => offset, SeekOrigin.Current => _pos + offset, _ => Length + offset };
    public override void Flush() { }
    public override void SetLength(long value) => throw new NotSupportedException();
    public override void Write(byte[] buffer, int offset, int count) => throw new NotSupportedException();
    protected override void Dispose(bool disposing) { if (disposing) foreach (var p in _parts) p.Dispose(); base.Dispose(disposing); }
}
