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
        var cands = new List<string>();
        foreach (var drive in new[] { "C", "D", "E", "F" })
        {
            cands.Add($@"{drive}:\Program Files (x86)\Rockstar Games\GTA San Andreas");
            cands.Add($@"{drive}:\Program Files\Rockstar Games\GTA San Andreas");
            cands.Add($@"{drive}:\Games\GTA San Andreas");
            cands.Add($@"{drive}:\GTA San Andreas");
            cands.Add($@"{drive}:\Games\Godjo RP");
            cands.Add($@"{drive}:\Program Files (x86)\Steam\steamapps\common\Grand Theft Auto San Andreas");
            cands.Add($@"{drive}:\SteamLibrary\steamapps\common\Grand Theft Auto San Andreas");
        }
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

    /// <summary>Запущена ли уже игра (мешает обновлению файлов).</summary>
    public static bool IsGameRunning()
    {
        try { return System.Diagnostics.Process.GetProcessesByName("gta_sa").Length > 0; } catch { return false; }
    }

    /// <summary>Версия gta_sa.exe по размеру файла (для подсказки игроку).</summary>
    public static string ExeVersion(string path)
    {
        try
        {
            var len = new FileInfo(Path.Combine(path, "gta_sa.exe")).Length;
            return len switch { 14383616 => "1.0 US", 14405632 => "1.0 EU", 15806464 => "1.01", 5697536 => "Steam 3.0", _ => "неизвестная" };
        }
        catch { return ""; }
    }

    /// <summary>Свободное место на диске с игрой, МБ.</summary>
    public static long FreeSpaceMb(string path)
    {
        try { return new DriveInfo(Path.GetPathRoot(Path.GetFullPath(path))!).AvailableFreeSpace / 1048576; } catch { return -1; }
    }

    public static System.Diagnostics.Process? Launch(string gtaPath, string ip, int port)
    {
        var psi = new System.Diagnostics.ProcessStartInfo(Path.Combine(gtaPath, "samp.exe"), $"{ip}:{port}")
        { WorkingDirectory = gtaPath, UseShellExecute = true };
        return System.Diagnostics.Process.Start(psi);
    }
}
