using System.IO.Compression;
using System.IO.Hashing;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Text.Json;

namespace GodjoLauncher;

/// <summary>Запись центрального каталога zip-архива сборки.</summary>
public sealed record RemoteEntry(string Name, uint Crc, long CompSize, long Size, int Method, long Offset);

/// <summary>
/// Чтение zip-архива сборки прямо с хостинга по HTTP Range, без скачивания целиком:
/// центральный каталог (имена, размеры, CRC32 всех файлов) весит десятки КБ,
/// а любой файл можно вытащить отдельным диапазоном байт. Части архива (game.zip.001, .002…)
/// склеиваются в одно адресное пространство.
/// </summary>
internal sealed class RemoteZip
{
    private readonly HttpClient _http;
    private readonly List<(long Start, long Size, Func<Task<string>> Url)> _parts = new();
    private readonly Dictionary<int, string> _resolved = new();
    public long Length { get; }

    public RemoteZip(HttpClient http, IEnumerable<(long Size, Func<Task<string>> Url)> parts)
    {
        _http = http;
        long acc = 0;
        foreach (var (size, url) in parts) { _parts.Add((acc, size, url)); acc += size; }
        Length = acc;
    }

    private async Task<string> Url(int i, bool fresh = false)
    {
        if (!fresh && _resolved.TryGetValue(i, out var u)) return u;
        return _resolved[i] = await _parts[i].Url();
    }

    /// <summary>Поток с байтами [off, off+len) архива; на границах частей переключается сам.</summary>
    public Stream Open(long off, long len, CancellationToken ct) => new RangeStream(this, off, len, ct);

    public async Task<byte[]> ReadAsync(long off, int len, CancellationToken ct)
    {
        var buf = new byte[len];
        await using var s = Open(off, len, ct);
        int got = 0, n;
        while (got < len && (n = await s.ReadAsync(buf.AsMemory(got, len - got), ct)) > 0) got += n;
        if (got != len) throw new IOException("Хостинг вернул неполные данные архива.");
        return buf;
    }

    private async Task<Stream> OpenPartAsync(int i, long from, long to, CancellationToken ct)
    {
        for (int attempt = 0; ; attempt++)
        {
            var req = new HttpRequestMessage(HttpMethod.Get, await Url(i, attempt > 0));
            req.Headers.Range = new RangeHeaderValue(from, to);
            var resp = await _http.SendAsync(req, HttpCompletionOption.ResponseHeadersRead, ct);
            if (resp.StatusCode == System.Net.HttpStatusCode.PartialContent)
                return await resp.Content.ReadAsStreamAsync(ct);
            bool whole = resp.StatusCode == System.Net.HttpStatusCode.OK && from == 0 && to == _parts[i].Size - 1;
            if (whole) return await resp.Content.ReadAsStreamAsync(ct);
            int code = (int)resp.StatusCode;
            resp.Dispose();
            // прямые ссылки OneDrive/Яндекса живут недолго — один раз пробуем получить новую
            if (attempt == 0 && code is 401 or 403 or 404 or 410) continue;
            if (resp.StatusCode == System.Net.HttpStatusCode.OK)
                throw new IOException("Хостинг сборки не поддерживает докачку отдельных файлов (HTTP Range).");
            throw new HttpRequestException($"Хостинг сборки ответил {code}.");
        }
    }

    private sealed class RangeStream : Stream
    {
        private readonly RemoteZip _z; private readonly CancellationToken _ct;
        private long _pos; private readonly long _end; private Stream? _cur; private long _curEnd;
        public RangeStream(RemoteZip z, long off, long len, CancellationToken ct) { _z = z; _pos = off; _end = off + len; _ct = ct; }
        public override async ValueTask<int> ReadAsync(Memory<byte> buffer, CancellationToken ct = default)
        {
            while (true)
            {
                if (_pos >= _end) return 0;
                if (_cur == null)
                {
                    int i = _z._parts.FindLastIndex(p => p.Start <= _pos);
                    var p = _z._parts[i];
                    _curEnd = Math.Min(_end, p.Start + p.Size);
                    _cur = await _z.OpenPartAsync(i, _pos - p.Start, _curEnd - p.Start - 1, _ct);
                }
                int want = (int)Math.Min(buffer.Length, _curEnd - _pos);
                int n = await _cur.ReadAsync(buffer[..want], _ct);
                if (n > 0) { _pos += n; if (_pos >= _curEnd) { _cur.Dispose(); _cur = null; } return n; }
                _cur.Dispose(); _cur = null;
                if (_pos < _curEnd) throw new IOException("Соединение с хостингом сборки прервалось.");
            }
        }
        public override int Read(byte[] buffer, int offset, int count) => ReadAsync(buffer.AsMemory(offset, count)).AsTask().GetAwaiter().GetResult();
        public override bool CanRead => true; public override bool CanSeek => false; public override bool CanWrite => false;
        public override long Length => _end; public override long Position { get => _pos; set => throw new NotSupportedException(); }
        public override void Flush() { }
        public override long Seek(long o, SeekOrigin s) => throw new NotSupportedException();
        public override void SetLength(long v) => throw new NotSupportedException();
        public override void Write(byte[] b, int o, int c) => throw new NotSupportedException();
        protected override void Dispose(bool d) { if (d) _cur?.Dispose(); base.Dispose(d); }
    }

    // ---------------- разбор zip ----------------
    private static uint U32(byte[] b, int o) => BitConverter.ToUInt32(b, o);
    private static ushort U16(byte[] b, int o) => BitConverter.ToUInt16(b, o);

    public async Task<List<RemoteEntry>> ReadDirectoryAsync(CancellationToken ct)
    {
        int tailLen = (int)Math.Min(Length, 65557 + 20);
        var tail = await ReadAsync(Length - tailLen, tailLen, ct);
        int e = -1;
        for (int i = tail.Length - 22; i >= 0; i--) if (U32(tail, i) == 0x06054b50) { e = i; break; }
        if (e < 0) throw new InvalidDataException("Архив сборки повреждён: не найден каталог файлов.");
        long count = U16(tail, e + 10), cdSize = U32(tail, e + 12), cdOff = U32(tail, e + 16);
        if ((count == 0xFFFF || cdSize == 0xFFFFFFFF || cdOff == 0xFFFFFFFF) && e >= 20 && U32(tail, e - 20) == 0x07064b50)
        {
            long e64 = BitConverter.ToInt64(tail, e - 20 + 8);
            var z = await ReadAsync(e64, 56, ct);
            if (U32(z, 0) != 0x06064b50) throw new InvalidDataException("Архив сборки повреждён (zip64).");
            count = BitConverter.ToInt64(z, 32); cdSize = BitConverter.ToInt64(z, 40); cdOff = BitConverter.ToInt64(z, 48);
        }
        var cd = await ReadAsync(cdOff, (int)cdSize, ct);
        var list = new List<RemoteEntry>((int)count);
        Encoding legacy;
        try { legacy = Encoding.GetEncoding(437); } catch { legacy = Encoding.Latin1; }
        for (int p = 0; p + 46 <= cd.Length && U32(cd, p) == 0x02014b50;)
        {
            int flags = U16(cd, p + 8), method = U16(cd, p + 10);
            uint crc = U32(cd, p + 16);
            long comp = U32(cd, p + 20), size = U32(cd, p + 24);
            int nl = U16(cd, p + 28), xl = U16(cd, p + 30), cl = U16(cd, p + 32);
            long off = U32(cd, p + 42);
            string name = ((flags & 0x800) != 0 ? Encoding.UTF8 : legacy).GetString(cd, p + 46, nl);
            for (int x = p + 46 + nl; x + 4 <= p + 46 + nl + xl;)
            {
                int id = U16(cd, x), len = U16(cd, x + 2), q = x + 4;
                if (id == 1)
                {
                    if (size == 0xFFFFFFFF) { size = BitConverter.ToInt64(cd, q); q += 8; }
                    if (comp == 0xFFFFFFFF) { comp = BitConverter.ToInt64(cd, q); q += 8; }
                    if (off == 0xFFFFFFFF) { off = BitConverter.ToInt64(cd, q); }
                }
                x += 4 + len;
            }
            list.Add(new RemoteEntry(name, crc, comp, size, method, off));
            p += 46 + nl + xl + cl;
        }
        return list;
    }

    /// <summary>Скачивает один файл из архива и атомарно кладёт его на место (с проверкой CRC32).</summary>
    public async Task ExtractAsync(RemoteEntry e, string dest, Action<long>? onBytes, CancellationToken ct)
    {
        var lh = await ReadAsync(e.Offset, 30, ct);
        if (U32(lh, 0) != 0x04034b50) throw new InvalidDataException("Архив сборки повреждён: " + e.Name);
        long data = e.Offset + 30 + U16(lh, 26) + U16(lh, 28);
        Directory.CreateDirectory(Path.GetDirectoryName(dest)!);
        var tmp = dest + ".godjo-tmp";
        var crc = new Crc32();
        await using (var raw = Open(data, e.CompSize, ct))
        await using (var counted = new CountingStream(raw, onBytes))
        await using (Stream src = e.Method == 8 ? new DeflateStream(counted, CompressionMode.Decompress) : counted)
        await using (var dst = new FileStream(tmp, FileMode.Create, FileAccess.Write, FileShare.None, 1 << 20))
        {
            var buf = new byte[1 << 20]; int n;
            while ((n = await src.ReadAsync(buf, ct)) > 0) { crc.Append(buf.AsSpan(0, n)); await dst.WriteAsync(buf.AsMemory(0, n), ct); }
        }
        if (crc.GetCurrentHashAsUInt32() != e.Crc) { try { File.Delete(tmp); } catch { } throw new InvalidDataException("Файл скачался с ошибкой: " + e.Name); }
        File.Move(tmp, dest, true);
    }

    private sealed class CountingStream : Stream
    {
        private readonly Stream _s; private readonly Action<long>? _cb;
        public CountingStream(Stream s, Action<long>? cb) { _s = s; _cb = cb; }
        public override async ValueTask<int> ReadAsync(Memory<byte> b, CancellationToken ct = default) { int n = await _s.ReadAsync(b, ct); if (n > 0) _cb?.Invoke(n); return n; }
        public override int Read(byte[] b, int o, int c) { int n = _s.Read(b, o, c); if (n > 0) _cb?.Invoke(n); return n; }
        public override bool CanRead => true; public override bool CanSeek => false; public override bool CanWrite => false;
        public override long Length => _s.Length; public override long Position { get => throw new NotSupportedException(); set => throw new NotSupportedException(); }
        public override void Flush() { }
        public override long Seek(long o, SeekOrigin s) => throw new NotSupportedException();
        public override void SetLength(long v) => throw new NotSupportedException();
        public override void Write(byte[] b, int o, int c) => throw new NotSupportedException();
    }
}

/// <summary>
/// Кэш контрольных сумм файлов игры (godjo\game-files.json): CRC32 пересчитывается
/// только для файлов, у которых изменился размер или дата — поэтому проверка при «Играть» занимает секунды.
/// </summary>
internal sealed class FileHashCache
{
    public sealed class Item { public long Size { get; set; } public long Time { get; set; } public uint Crc { get; set; } }
    private readonly string _path;
    public Dictionary<string, Item> Items { get; }

    private FileHashCache(string path, Dictionary<string, Item> items) { _path = path; Items = items; }

    public static FileHashCache Load(string gta)
    {
        var p = Path.Combine(gta, "godjo", "game-files.json");
        try { if (File.Exists(p)) return new(p, JsonSerializer.Deserialize<Dictionary<string, Item>>(File.ReadAllText(p)) ?? new()); }
        catch (Exception ex) { Log.Write("game-files.json: " + ex.Message); }
        return new(p, new Dictionary<string, Item>(StringComparer.OrdinalIgnoreCase));
    }

    public void Save()
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_path)!);
            File.WriteAllText(_path, JsonSerializer.Serialize(Items));
        }
        catch (Exception ex) { Log.Write("game-files.json save: " + ex.Message); }
    }

    public void Put(string rel, string full, uint crc)
    {
        var fi = new FileInfo(full);
        Items[Key(rel)] = new Item { Size = fi.Length, Time = fi.LastWriteTimeUtc.Ticks, Crc = crc };
    }

    /// <summary>CRC32 файла: из кэша, если файл не менялся, иначе считаем заново.</summary>
    public async Task<uint> CrcAsync(string rel, string full, CancellationToken ct)
    {
        var fi = new FileInfo(full);
        if (Items.TryGetValue(Key(rel), out var it) && it.Size == fi.Length && it.Time == fi.LastWriteTimeUtc.Ticks) return it.Crc;
        uint crc = await ComputeAsync(full, ct);
        Items[Key(rel)] = new Item { Size = fi.Length, Time = fi.LastWriteTimeUtc.Ticks, Crc = crc };
        return crc;
    }

    public static async Task<uint> ComputeAsync(string full, CancellationToken ct)
    {
        var crc = new Crc32();
        await using var fs = new FileStream(full, FileMode.Open, FileAccess.Read, FileShare.ReadWrite, 1 << 20, FileOptions.SequentialScan);
        var buf = new byte[1 << 20]; int n;
        while ((n = await fs.ReadAsync(buf, ct)) > 0) crc.Append(buf.AsSpan(0, n));
        return crc.GetCurrentHashAsUInt32();
    }

    private static string Key(string rel) => rel.Replace('/', '\\').ToLowerInvariant();
}
