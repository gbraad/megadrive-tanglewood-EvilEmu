using System.IO;
using System.Collections.Generic;
using Sharpmake;

[module: Sharpmake.Include("ion/ion.make.cs")]
[module: Sharpmake.Include("ion/assets.make.cs")]

[Generate]
class EvilEmu : IonExe
{
    private enum GameType
    {
        Demo,
        Final
    }

    private GameType gameType = GameType.Demo;

    public EvilEmu() : base("EvilEmu", "evil_emu")
    {
        AddTargets(Globals.IonTargetsDefault);
        SourceFilesExtensions.Add(".png");
    }

    protected override void ExcludeOutputFiles()
    {
        base.ExcludeOutputFiles();
        BuildTexture.ConfigureTextures(this);
    }

    [Configure]
    public override void Configure(Project.Configuration conf, Target target)
    {
        base.Configure(conf, target);

        // ion::engine
        conf.AddPublicDependency<IonEngine>(target);

        // Include directories
        conf.IncludePaths.Add(@"[project.SharpmakeCsPath]/evil_emu/megaex");

        // Determine game build type
        if(gameType == GameType.Demo)
            conf.Defines.Add("EVIL_EMU_GAME_TYPE=EVIL_EMU_GAME_TYPE_DEMO");
        else
            conf.Defines.Add("EVIL_EMU_GAME_TYPE=EVIL_EMU_GAME_TYPE_FINAL");

        // Configure extended Mega Drive features
		conf.Defines.Add("VDP_H50_MODE=0");
		conf.Defines.Add("VDP_H54_MODE=1");
		conf.Defines.Add("VDP_H106_MODE=0");
		conf.Defines.Add("VRAM_128KB_MODE=1");
		conf.Defines.Add("EMU_SUPPORT_8MB_ROM=1");

        // Build as master build (embeds ROM into executable)
        conf.Defines.Add("ION_BUILD_MASTER=1");

        // Assets
        conf.TargetCopyFilesToSubDirectory.Add(new KeyValuePair<string, string>(@"[project.SharpmakeCsPath]/evil_emu/assets/shaders", "bin/assets/shaders"));
        conf.TargetCopyFilesToSubDirectory.Add(new KeyValuePair<string, string>(@"[project.SharpmakeCsPath]/evil_emu/assets/textures", "bin/assets/textures"));
        conf.TargetCopyFilesToSubDirectory.Add(new KeyValuePair<string, string>(@"[project.SharpmakeCsPath]/evil_emu/assets/fonts", "bin/assets/fonts"));
        conf.TargetCopyFilesToSubDirectory.Add(new KeyValuePair<string, string>(@"[project.SharpmakeCsPath]/evil_emu/assets/materials", "bin/assets/materials"));
        conf.TargetCopyFilesToSubDirectory.Add(new KeyValuePair<string, string>(@"[project.SharpmakeCsPath]/evil_emu/roms", "bin/roms"));

        // Debugger
        conf.VcxprojUserFile = new Project.Configuration.VcxprojUserFileSettings { LocalDebuggerWorkingDirectory = @"[conf.TargetPath]/bin" };

        // Platform spec files
        //conf.TargetCopyFilesToSubDirectory.Add(new KeyValuePair<string, string>(@"[project.SharpmakeCsPath]/evil_emu/platform/switch/SpecFiles", "SpecFiles"));
        conf.MetadataSource = @"[project.SharpmakeCsPath]/evil_emu/platform/switch/SpecFiles/Application.nmeta";
    }
}

[Generate]
class EvilEmuSolution : IonSolution
{
    public EvilEmuSolution() : base("EvilEmu")
    {
        AddTargets(Globals.IonTargetsDefault);
    }

    [Configure]
    public override void Configure(Solution.Configuration conf, Target target)
    {
        base.Configure(conf, target);

        // Game
        conf.AddProject<EvilEmu>(target);

        // Tools
        if (target.Platform == Platform.win64 && (target.Optimization == Optimization.Debug || target.Optimization == Optimization.Release))
        {
            conf.AddProject<IonBuildResource>(target);
        }
    }
}

public static class BuildMain
{
    [Main]
    public static void SharpmakeMain(Arguments sharpmakeArgs)
    {
        sharpmakeArgs.Generate<EvilEmuSolution>();
    }
}