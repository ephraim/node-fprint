{
	"targets": [
		{
			"target_name": "fingerprint",
			"sources": [ "src/enroll.cpp", "src/verify.cpp", "src/fingerprint.cpp" ],
			"include_dirs": [
				"<!(node -e \"require('nan')\")",
				"<!(pkg-config --cflags libfprint)"
			],
			"libraries": [ "<!(pkg-config --libs libfprint)" ],
			"variables": {
				"node_version": '<!(node --version | sed -e "s/^v\([0-9]*\\.[0-9]*\).*$/\\1/")'
			},
			"target_conditions": [
				[ "node_version == '0.8'", { "defines": ["OLD_UV_RUN_SIGNATURE"] } ]
			]
		}
	],
}
