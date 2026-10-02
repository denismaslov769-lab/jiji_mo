using System.Text.Json;
using System.Text.Json.Serialization;

namespace GodjoLauncher;

/// <summary>Пользовательские настройки (%APPDATA%\GodjoLauncher\config.json).</summary>
public sealed class Config
{
    public string GtaPath { get; set; } = "";
    public string Nick { get; set; } = "";
    public string ServerIp { get; set; } = "";
    public int ServerPort { get; set; } = 0;
    public bool CloseOnPlay { get; set; } = true;
    /// <summary>Включённые/выключенные дополнительные моды сборки (name → вкл).</summary>
    public Dictionary<string, bool> Mods { get; set; } = new();
    /// <summary>Последние ники (быстрый выбор).</summary>
    public List<string> NickHistory { get; set; } = new();
    public bool ShowLoadingScreen { get; set; } = true;
    public bool PlaySounds { get; set; } = true;

    [JsonIgnore] public static string Dir => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "GodjoLauncher");
    [JsonIgnore] public static string FilePath => Path.Combine(Dir, "config.json");

    public static Config Load()
    {
        try
        {
            if (File.Exists(FilePath)) return JsonSerializer.Deserialize<Config>(File.ReadAllText(FilePath)) ?? new Config();
        }
        catch (Exception ex) { Log.Write("config load: " + ex.Message); }
        return new Config();
    }

    public void Save()
    {
        try
        {
            Directory.CreateDirectory(Dir);
            File.WriteAllText(FilePath, JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true }));
        }
        catch (Exception ex) { Log.Write("config save: " + ex.Message); }
    }

    public void RememberNick(string nick)
    {
        if (string.IsNullOrWhiteSpace(nick)) return;
        NickHistory.RemoveAll(n => string.Equals(n, nick, StringComparison.OrdinalIgnoreCase));
        NickHistory.Insert(0, nick);
        if (NickHistory.Count > 5) NickHistory.RemoveRange(5, NickHistory.Count - 5);
    }
}

/// <summary>Настройки дистрибутива (launcher.json рядом с exe) — задаёт владелец сервера.</summary>
public sealed class LauncherSettings
{
    public string ManifestUrl { get; set; } = "https://github.com/denismaslov769-lab/jiji_mo/releases/latest/download/manifest.json";
    public string ServerName { get; set; } = "Godjo Role Play";
    public string ServerIp { get; set; } = "127.0.0.1";
    public int ServerPort { get; set; } = 7777;
    public string Site { get; set; } = "https://github.com/denismaslov769-lab/jiji_mo";
    public string Discord { get; set; } = "";
    public string Vk { get; set; } = "";
    public string Telegram { get; set; } = "";

    public static LauncherSettings Load()
    {
        try
        {
            var p = Path.Combine(AppContext.BaseDirectory, "launcher.json");
            if (File.Exists(p)) return JsonSerializer.Deserialize<LauncherSettings>(File.ReadAllText(p), new JsonSerializerOptions { PropertyNameCaseInsensitive = true }) ?? new();
        }
        catch (Exception ex) { Log.Write("launcher.json: " + ex.Message); }
        return new LauncherSettings();
    }
}

/// <summary>Простой журнал лаунчера: %APPDATA%\GodjoLauncher\launcher.log (до 512 КБ).</summary>
public static class Log
{
    private static readonly object Lock = new();
    public static string FilePath => Path.Combine(Config.Dir, "launcher.log");

    public static void Write(string text)
    {
        try
        {
            lock (Lock)
            {
                Directory.CreateDirectory(Config.Dir);
                var fi = new FileInfo(FilePath);
                if (fi.Exists && fi.Length > 512 * 1024) fi.Delete();
                File.AppendAllText(FilePath, $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] {text}{Environment.NewLine}");
            }
        }
        catch { }
    }
}
