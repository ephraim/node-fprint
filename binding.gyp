{
	"targets":
		[{
			"target_name": "fingerprint",
			"sources": [ "src/fingerprint.cpp" ],
			"cflags": [ "-std=c++11" ],
			"include_dirs": [ "<!(node -e \"require('nan')\")", "<!(pkg-config --cflags libfprint)" ],
			"libraries": [ "<!(pkg-config --libs libfprint)" ]
		}],
}
