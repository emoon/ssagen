Build {
	Units = "units.lua",

	Configs = {
		{
			Name = "macosx-gcc",
			DefaultOnHost = "macosx",
			Tools = { "clang-osx" },
			Env = {
				CPPOPTS = {
					{ "-g", "-O0"; Config = "macosx-gcc-debug" },
				},
				CCOPTS = {
					{ "-g", "-O0"; Config = "macosx-gcc-debug" },
				},
			},
		},
	},
}
