{
    "targets": 
    [
        {
            "target_name":  "skeppleton",
            "sources":      [ "src/skeppleton.cpp" ],
            "include_dirs": [ "<!(node -e \"require('nan')\")" ]
        }
    ]
}