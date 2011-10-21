
Program {
	Name = "test",

	Env = {
		CCOPTS = {
			{ "-Werror", "-pedantic-errors", "-Wall"; Config = "macosx-gcc-debug" },
		},
	},

	Sources = { "ssagen.c" }, 
}

Default "test"

