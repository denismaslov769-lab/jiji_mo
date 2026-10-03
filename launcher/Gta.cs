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
        var missing = MissingCoreFiles(path);
        if (missing.Count > 0)
            return new(false, true, samp, "", false, "Игра повреждена: нет " + string.Join(", ", missing) +
                ". Восстановите их из карантина антивируса или переустановите GTA SA (обычно их удаляет Защитник Windows)");
        if (!samp) return new(false, true, false, "", false, "Не установлен SA-MP 0.3.7 (нет samp.exe / samp.dll)");
        var ver = DetectSampVersion(Path.Combine(path, "samp.dll"));
        bool sup = Supported.Contains(ver);
        return new(sup, true, true, ver, sup, sup ? $"SA-MP {ver} — подходит" : $"SA-MP {ver} не поддерживается интерфейсом. Нужен клиент 0.3.7-R1 или 0.3.7-R3");
    }

    // без этих файлов gta_sa.exe не запускается («vorbisFile.dll не обнаружен», «eax.dll не обнаружен»)
    public static readonly string[] CoreFiles = { "vorbisFile.dll", "vorbis.dll", "ogg.dll", "eax.dll", "stream.ini", @"models\gta3.img", @"data\gta.dat" };
    public static List<string> MissingCoreFiles(string path) =>
        CoreFiles.Where(f => !File.Exists(Path.Combine(path, f))).ToList();

    public static string GuessPath()
    {
        try
        {
            using var k = Registry.CurrentUser.OpenSubKey(@"Software\SAMP");
            var exe = k?.GetValue("gta_sa_exe") as string;
            if (!string.IsNullOrEmpty(exe) && File.Exists(exe)) return Path.GetDirectoryName(exe)!;
        }
        catch { }
        var cands = new List<string> { Path.Combine(AppContext.BaseDirectory, "game") };
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

    /// <summary>
    /// Запуск игры напрямую: gta_sa.exe стартует «замороженным», в него подгружается samp.dll
    /// (как это делает samp.exe), окно браузера серверов SA-MP не появляется.
    /// Если внедрить не удалось — запасной путь через samp.exe.
    /// </summary>
    public static System.Diagnostics.Process? Launch(string gtaPath, string ip, int port, string nick = "", string password = "")
    {
        if (string.IsNullOrEmpty(nick)) nick = GetNick();
        try
        {
            var p = LaunchDirect(gtaPath, ip, port, nick, password);
            if (p != null) { Log.Write("launch: direct gta_sa.exe + samp.dll"); return p; }
        }
        catch (Exception e) { Log.Write("launch direct failed: " + e.Message); }
        var psi = new System.Diagnostics.ProcessStartInfo(Path.Combine(gtaPath, "samp.exe"), $"{ip}:{port}" + (string.IsNullOrEmpty(password) ? "" : " " + password))
        { WorkingDirectory = gtaPath, UseShellExecute = true };
        Log.Write("launch: fallback samp.exe");
        return System.Diagnostics.Process.Start(psi);
    }

    private static System.Diagnostics.Process? LaunchDirect(string gtaPath, string ip, int port, string nick, string password)
    {
        // gta_sa.exe — 32-битный процесс; LoadLibraryW берём из своего kernel32, поэтому лаунчер собран под x86
        if (Environment.Is64BitProcess) { Log.Write("launch direct: launcher is x64, skip"); return null; }
        string exe = Path.Combine(gtaPath, "gta_sa.exe"), dll = Path.Combine(gtaPath, "samp.dll");
        if (!File.Exists(exe) || !File.Exists(dll)) return null;
        var args = $"\"{exe}\" -c -n {nick} -h {ip} -p {port}" + (string.IsNullOrEmpty(password) ? "" : $" -z {password}");
        var si = new Native.STARTUPINFO { cb = System.Runtime.InteropServices.Marshal.SizeOf<Native.STARTUPINFO>() };
        if (!Native.CreateProcess(exe, new System.Text.StringBuilder(args), IntPtr.Zero, IntPtr.Zero, false, Native.CREATE_SUSPENDED, IntPtr.Zero, gtaPath, ref si, out var pi))
            throw new System.ComponentModel.Win32Exception();
        bool ok = false;
        try
        {
            var bytes = System.Text.Encoding.Unicode.GetBytes(dll + "\0");
            var mem = Native.VirtualAllocEx(pi.hProcess, IntPtr.Zero, (uint)bytes.Length, 0x3000, 0x04);
            if (mem == IntPtr.Zero) throw new System.ComponentModel.Win32Exception();
            if (!Native.WriteProcessMemory(pi.hProcess, mem, bytes, (uint)bytes.Length, out _)) throw new System.ComponentModel.Win32Exception();
            var ll = Native.GetProcAddress(Native.GetModuleHandle("kernel32.dll"), "LoadLibraryW");
            var th = Native.CreateRemoteThread(pi.hProcess, IntPtr.Zero, 0, ll, mem, 0, out _);
            if (th == IntPtr.Zero) throw new System.ComponentModel.Win32Exception();
            Native.WaitForSingleObject(th, 15000);
            Native.GetExitCodeThread(th, out var mod);
            Native.CloseHandle(th);
            Native.VirtualFreeEx(pi.hProcess, mem, 0, 0x8000);
            if (mod == 0) throw new Exception("samp.dll не загрузилась в процесс игры");
            Native.ResumeThread(pi.hThread);
            ok = true;
            return System.Diagnostics.Process.GetProcessById((int)pi.dwProcessId);
        }
        finally
        {
            if (!ok) Native.TerminateProcess(pi.hProcess, 1);
            Native.CloseHandle(pi.hThread); Native.CloseHandle(pi.hProcess);
        }
    }

    private static class Native
    {
        public const uint CREATE_SUSPENDED = 0x4;
        [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        public struct STARTUPINFO
        {
            public int cb; public string? lpReserved, lpDesktop, lpTitle;
            public int dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags;
            public short wShowWindow, cbReserved2; public IntPtr lpReserved2, hStdInput, hStdOutput, hStdError;
        }
        [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
        public struct PROCESS_INFORMATION { public IntPtr hProcess, hThread; public uint dwProcessId, dwThreadId; }
        [System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern bool CreateProcess(string app, System.Text.StringBuilder cmd, IntPtr pa, IntPtr ta, bool inherit, uint flags, IntPtr env, string dir, ref STARTUPINFO si, out PROCESS_INFORMATION pi);
        [System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr VirtualAllocEx(IntPtr h, IntPtr addr, uint size, uint type, uint prot);
        [System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool VirtualFreeEx(IntPtr h, IntPtr addr, uint size, uint type);
        [System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool WriteProcessMemory(IntPtr h, IntPtr addr, byte[] buf, uint size, out IntPtr written);
        [System.Runtime.InteropServices.DllImport("kernel32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern IntPtr GetModuleHandle(string name);
        [System.Runtime.InteropServices.DllImport("kernel32.dll", CharSet = System.Runtime.InteropServices.CharSet.Ansi)]
        public static extern IntPtr GetProcAddress(IntPtr mod, string name);
        [System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr CreateRemoteThread(IntPtr h, IntPtr attr, uint stack, IntPtr start, IntPtr param, uint flags, out uint id);
        [System.Runtime.InteropServices.DllImport("kernel32.dll")]
        public static extern uint WaitForSingleObject(IntPtr h, uint ms);
        [System.Runtime.InteropServices.DllImport("kernel32.dll")]
        public static extern bool GetExitCodeThread(IntPtr h, out uint code);
        [System.Runtime.InteropServices.DllImport("kernel32.dll")]
        public static extern uint ResumeThread(IntPtr h);
        [System.Runtime.InteropServices.DllImport("kernel32.dll")]
        public static extern bool TerminateProcess(IntPtr h, uint code);
        [System.Runtime.InteropServices.DllImport("kernel32.dll")]
        public static extern bool CloseHandle(IntPtr h);
    }
}
