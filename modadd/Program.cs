using Fantome.Libraries.League.IO.WAD;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace modadd
{
    class Program
    {
        static Dictionary<ulong, List<string>> GenerateLookup(string path)
        {
            var lookup = new Dictionary<ulong, List<string>>();
            foreach (var file in Directory.EnumerateFiles($"{path}/DATA/FINAL", "*.wad.client", SearchOption.AllDirectories))
            {
                using (var wad = new WADFile(File.OpenRead(file)))
                {
                    var name = Path.GetFileName(file);
                    foreach (var hash in wad.Entries.Select(e => e.XXHash))
                    {
                        if (!lookup.ContainsKey(hash))
                        {
                            lookup[hash] = new List<string>();
                        }
                        lookup[hash].Add(name);
                    }
                }
            }
            return lookup;
        }

        static string LookupName(Dictionary<ulong, List<string>> lookup, string path)
        {
            var found = new Dictionary<string, int>();
            var wad = new WADFile(path);

            foreach (var hash in wad.Entries.Select(e => e.XXHash))
            {
                if (lookup.ContainsKey(hash))
                {
                    foreach (var name in lookup[hash])
                    {
                        if (!found.ContainsKey(name))
                        {
                            found[name] = 1;
                        }
                        else
                        {
                            found[name] += 1;
                        }
                    }
                }
            }

            if (found.Count == 0)
            {
                return "Bootstrap.wad.client";
            }
            return found.OrderByDescending(x => x.Value).First().Key;
        }

        static string IsLeague(string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                return "";
            }
            if (path.EndsWith("League of Legends.exe"))
            {
                path = path.Replace("League of Legends.exe", "");
            }
            if (path.EndsWith("LeagueClient.exe"))
            {
                path = path.Replace("LeagueClient.exe", "");
            }
            if (path.EndsWith("LeagueClientUx.exe"))
            {
                path = path.Replace("LeagueClientUx.exe", "");
            }
            if (path.EndsWith("LeagueClientUxRender.exe"))
            {
                path = path.Replace("LeagueClientUxRender.exe", "");
            }
            if (File.Exists(path + "/League of Legends.exe"))
            {
                return path;
            }
            if (File.Exists(path + "/Game/League of Legends.exe"))
            {
                return path + "/Game";
            }
            return "";
        }

        static string ReadLine(string path, string defaultValue = "")
        {
            if(!File.Exists(path))
            {
                return defaultValue;
            }
            var lines = File.ReadAllLines(path);
            if(lines.Length == 0)
            {
                return defaultValue;
            }
            var line = lines[0];
            if (string.IsNullOrEmpty(line))
            {
                return defaultValue;
            }
            return line;
        }

        static void WriteLine(string path, string line)
        {
            File.WriteAllLines(path, new string[] { line });
        }

        static string FindLeague(string configPath, string path)
        {
            path = IsLeague(path);
            if (string.IsNullOrEmpty(path))
            {
                path = ReadLine(configPath);
                path = IsLeague(path);
            }

            if (string.IsNullOrEmpty(path))
            {
                path = IsLeague("C:\\Riot Games\\League of Legends\\Game");
            }

            if (string.IsNullOrEmpty(path))
            {
                using (var openFileDialog = new OpenFileDialog())
                {
                    openFileDialog.InitialDirectory = "C:\\Riot Games\\League of Legends\\Game";
                    openFileDialog.Filter = "League of Legends|League of Legends.exe|LeagueClient|LeagueClient.exe|LeagueClientUx|LeagueClientUx.exe|LeagueClientUxRenderer|LeagueClientUxRenderer.exe";
                    openFileDialog.RestoreDirectory = true;

                    if (openFileDialog.ShowDialog() == DialogResult.OK)
                    {
                        path = IsLeague(openFileDialog.FileName);
                    }
                }
            }

            if (!string.IsNullOrEmpty(path))
            {
                WriteLine(configPath, path);
            }
            return path;
        }
        
        static string SelectWad(string path)
        {
            if(!path.EndsWith(".wad.client") && !path.EndsWith(".wad"))
            {
                path = "";
                using (var openFileDialog = new OpenFileDialog())
                {
                    openFileDialog.InitialDirectory = "C:\\Riot Games\\League of Legends\\Game";
                    openFileDialog.Filter = "*.wad.client|*.wad.client|*.wad|*.wad|*.wad.mobile|*.wad.mobile";
                    openFileDialog.RestoreDirectory = true;

                    if (openFileDialog.ShowDialog() == DialogResult.OK)
                    {
                        path = openFileDialog.FileName;
                    }
                }
            }
            if (string.IsNullOrEmpty(path) || !File.Exists(path))
            {
                return "";
            }
            return path;
        }

        static void ExportWad(string source, string destDir, string wadname)
        {
            if (!Directory.Exists(destDir))
            {
                Directory.CreateDirectory(destDir);
            }
            var modname = Path.GetFileName(source);
            Console.WriteLine($"Exporting: {modname}");
            using (var fileStream = new FileStream($"{destDir}/{modname}.zip", FileMode.Create))
            using (var archive = new ZipArchive(fileStream, ZipArchiveMode.Create))
            {
                archive.CreateEntry("RAW\\");
                using (var meta = new StreamWriter(archive.CreateEntry("META\\info.json").Open()))
                {
                    meta.WriteLine("{");
                    meta.WriteLine($"    \"Name\": \"{modname}\",");
                    meta.WriteLine($"    \"Author\": \"Unknown\",");
                    meta.WriteLine($"    \"Version\": \"1.0.0\",");
                    meta.WriteLine($"    \"Description\": \"Exported from lolcustomskin-tools\"");
                    meta.WriteLine("}");
                }
                using (var infile = File.OpenRead(source))
                {
                    var dest = $"WAD\\{wadname}";
                    using (var wadEntry = archive.CreateEntry(dest.Replace('/', '\\')).Open())
                    {
                        infile.CopyTo(wadEntry);
                    }
                }
            }
        }

        [STAThread]
        static void Main(string[] args)
        {
            try
            {
                var exeDir = Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);
                var lolDir = FindLeague($"{exeDir}/wadinstall.txt", args.Length > 0 ? args[0] : "");
                if (string.IsNullOrEmpty(lolDir))
                {
                    MessageBox.Show("No valid league install found!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                var lookup = GenerateLookup(lolDir);
                var source = SelectWad(args.Length > 0 ? args[0] : "");
                if (string.IsNullOrEmpty(source))
                {
                    MessageBox.Show("No valid league wad selected!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                var exportDir = $"{exeDir}/ConvertedMods";
                var wadname = LookupName(lookup, source);
                ExportWad(source, exportDir, wadname);
                Process.Start($"{exportDir}/");
                MessageBox.Show("Mod has been sucesfully converted!", "Finished", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            catch(Exception error)
            {
                MessageBox.Show(error.StackTrace, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
