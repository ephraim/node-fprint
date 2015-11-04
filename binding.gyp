{
	"targets":
		[{
			"target_name": "fingerprint",
			"sources": [ "src/enroll.cpp", "src/fingerprint.cpp" ],
			"include_dirs": [
				"<!(node -e \"require('nan')\")",
				"<!(pkg-config --cflags libfprint)"
			],
			"libraries": [ "<!(pkg-config --libs libfprint)" ]
		}],
}
