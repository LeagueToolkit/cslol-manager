using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace modapply
{
    class Program
    {
        static string RelativePath(string root, string path)
        {
            var uri_root = new Uri(root);
            var uri_path = new Uri(path);
            var rel_uri = uri_root.MakeRelativeUri(uri_path);
            return rel_uri.OriginalString;
        }

        static void ExportFile(ZipArchive archive, string source, string dest)
        {
            using (var infile = new FileStream(source, FileMode.Open))
            {
                using (var wadEntry = archive.CreateEntry(dest.Replace('/', '\\')).Open())
                {
                    infile.CopyTo(wadEntry);
                }
            }
        }

        static void ExportFolder(ZipArchive archive, string source, string dest)
        {
            foreach (var file in Directory.EnumerateFiles(source, "*.*", SearchOption.AllDirectories))
            {
                var rel = RelativePath(source, file);
                ExportFile(archive, file, $"{dest}/{rel}");
            }
        }

        static void ExportMod(string sourceDir, string destDir)
        {
            if (!Directory.Exists(destDir))
            {
                Directory.CreateDirectory(destDir);
            }
            var modname = Path.GetFileName(sourceDir);
            Console.WriteLine($"Exporting: {modname}");
            using (var fileStream = new FileStream($"{destDir}/{modname}.zip", FileMode.Create))
            using (var archive = new ZipArchive(fileStream, ZipArchiveMode.Create))
            {
                archive.CreateEntry("RAW/");
                using (var meta = new StreamWriter(archive.CreateEntry("META/info.json").Open()))
                {
                    meta.WriteLine("{");
                    meta.WriteLine($"    \"Name\": \"{modname}\",");
                    meta.WriteLine($"    \"Author\": \"Unknown\",");
                    meta.WriteLine($"    \"Version\": \"1.0.0\",");
                    meta.WriteLine($"    \"Description\": \"Exported from lolcustomskin-tools\"");
                    meta.WriteLine("}");
                }
                foreach (var wad in Directory.EnumerateFileSystemEntries(sourceDir, "*.wad.client", SearchOption.AllDirectories))
                {
                    var wadname = Path.GetFileName(wad);
                    if (File.GetAttributes(wad).HasFlag(FileAttributes.Directory))
                    {
                        ExportFolder(archive, wad, $"WAD");
                    }
                    else
                    {
                        ExportFile(archive, wad, $"WAD/{wadname}");
                    }
                }
            }
        }

        static void Main(string[] args)
        {
            var exeDir = Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);
            Console.WriteLine("This tool will now convert all your mods to Fantome format!");
            Console.WriteLine("Press enter to continue...");
            Console.ReadLine();

            int modcount = 0;
            var importDir = $"{exeDir}/mods";
            var exportDir = $"{exeDir}/ExportedMods";
            if (Directory.Exists(importDir))
            {
                foreach (var directory in Directory.EnumerateDirectories(importDir))
                {
                    ExportMod(directory, exportDir);
                    modcount++;
                }
                Directory.Move(importDir, importDir + "_backup");
            }

            if (modcount > 0)
            {
                Console.WriteLine($"Exporeted {modcount} mods!");
                Console.WriteLine($"You can find your converted mods in: {exportDir}/");
                Process.Start($"{exportDir}/");
            }
            else
            {
                Console.WriteLine("No mods installed!");
            }
            Console.WriteLine("Press enter to continue...");
            Console.ReadLine();
        }
    }
}
