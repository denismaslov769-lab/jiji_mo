using Microsoft.Win32;

namespace GodjoLauncher;

public sealed record GtaCheck(bool Ok, bool HasGta, bool HasSamp, string SampVersion, bool SampSupported, string Message);

public static class Gta
{
    // Точки входа samp.dll (AddressOfEntryPoint) для известных версий клиента 0.3.7
    private static readonly Dictionary<uint, string> Versions = new()
    {
        [0x31DF13] = "0.3.7-R1", [0x3195DD] = "0.3.7-R2", [0xCC4D0] = "0.3.7-R3-1", [0xCBCB0] = "0.3.7-R4",
        [0xCBCD0] = "0.3.7-R4-2", [0xCBC90] = "0.3.7-R5", [0xFDB60] = "0.3.DL"
    };
    // samp-cef поддерживает клиенты R1 и R3
    private static readonly HashSet<string> Supported = new() { "0.3.7-R1", "0.3.7-R3-1" };

    public static string DetectSampVersion(string sampDll)
    {
        try
        {
            using var fs = File.OpenRead(sampDll);
            using var br = new BinaryReader(fs);
            fs.Seek(0x3C, SeekOrigin.Begin);
            int pe = br.ReadInt32();
            fs.Seek(pe + 0x28, SeekOrigin.Begin);
            uint ep = br.ReadUInt32();
            return Versions.TryGetValue(ep, out var v) ? v : $"неизвестная (0x{ep:X})";
        }
        catch { return "не определена"; }
    }

    public static GtaCheck Check(string path)
    {
        if (string.IsNullOrWhiteSpace(path) || !Directory.Exists(path))
            return new(false, false, false, "", false, "Укажите папку с GTA San Andreas");
        bool gta = File.Exists(Path.Combine(path, "gta_sa.exe"));
        bool samp = File.Exists(Path.Combine(path, "samp.exe")) && File.Exists(Path.Combine(path, "samp.dll"));
        if (!gta) return new(false, false, samp, "", false, "В папке нет gta_sa.exe");
        if (!samp) return new(false, true, false, "", false, "Не установлен SA-MP 0.3.7 (нет samp.exe / samp.dll)");
        var ver = DetectSampVersion(Path.Combine(path, "samp.dll"));
        bool sup = Supported.Contains(ver);
        return new(sup, true, true, ver, sup, sup ? $"SA-MP {ver} — подходит" : $"SA-MP {ver} не поддерживается интерфейсом. Нужен клиент 0.3.7-R1 или 0.3.7-R3");
    }

    public static string GuessPath()
    {
        try
        {
            using var k = Registry.CurrentUser.OpenSubKey(@"Software\SAMP");
            var exe = k?.GetValue("gta_sa_exe") as string;
            if (!string.IsNullOrEmpty(exe) && File.Exists(exe)) return Path.GetDirectoryName(exe)!;
        }
        catch { }
        string[] cands =
        {
            @"C:\Program Files (x86)\Rockstar Games\GTA San Andreas", @"C:\Program Files\Rockstar Games\GTA San Andreas",
            @"C:\Games\GTA San Andreas", @"D:\Games\GTA San Andreas", @"C:\GTA San Andreas", @"D:\GTA San Andreas"
        };
        return cands.FirstOrDefault(c => File.Exists(Path.Combine(c, "gta_sa.exe"))) ?? "";
    }

    public static void SetNick(string nick)
    {
        using var k = Registry.CurrentUser.CreateSubKey(@"Software\SAMP");
        k.SetValue("PlayerName", nick);
    }

    public static string GetNick()
    {
        try { using var k = Registry.CurrentUser.OpenSubKey(@"Software\SAMP"); return k?.GetValue("PlayerName") as string ?? ""; }
        catch { return ""; }
    }

    public static bool IsRpNick(string n)
    {
        if (n.Length < 5 || n.Length > 24) return false;
        var parts = n.Split('_');
        if (parts.Length != 2) return false;
        foreach (var p in parts)
        {
            if (p.Length < 2 || !char.IsUpper(p[0]) || p[0] > 'Z') return false;
            if (!p.Skip(1).All(c => c >= 'a' && c <= 'z')) return false;
        }
        return true;
    }

    public static void Launch(string gtaPath, string ip, int port)
    {
        var psi = new System.Diagnostics.ProcessStartInfo(Path.Combine(gtaPath, "samp.exe"), $"{ip}:{port}")
        { WorkingDirectory = gtaPath, UseShellExecute = true };
        System.Diagnostics.Process.Start(psi);
    }
}
