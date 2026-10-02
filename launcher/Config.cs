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

    [JsonIgnore] public static string Dir => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "GodjoLauncher");
    [JsonIgnore] public static string FilePath => Path.Combine(Dir, "config.json");

    public static Config Load()
    {
        try
        {
            if (File.Exists(FilePath)) return JsonSerializer.Deserialize<Config>(File.ReadAllText(FilePath)) ?? new Config();
        }
        catch { }
        return new Config();
    }

    public void Save()
    {
        Directory.CreateDirectory(Dir);
        File.WriteAllText(FilePath, JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true }));
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

    public static LauncherSettings Load()
    {
        try
        {
            var p = Path.Combine(AppContext.BaseDirectory, "launcher.json");
            if (File.Exists(p)) return JsonSerializer.Deserialize<LauncherSettings>(File.ReadAllText(p), new JsonSerializerOptions { PropertyNameCaseInsensitive = true }) ?? new();
        }
        catch { }
        return new LauncherSettings();
    }
}
