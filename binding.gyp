{
  "targets": [
    {
      "target_name": "raspicam2",
      "sources": [ "raspicam2.cc" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
	"/usr/local/include/raspicam"
      ],
      "link_settings": {
        "libraries": [ 
          "-lraspicam",
          "-lraspicam_cv"
      	],
        'ldflags': ['-L/usr/local/lib']
      }
    }
  ]
}
