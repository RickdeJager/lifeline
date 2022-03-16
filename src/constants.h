// Ideally, these should be shorter or equal to the original process name length.
// lifeline\0 --> 9
//
// Forcefully setting a larger process name _is_ possible, but at that point you start overwriting
// envp entries, which quickly risks crashes in miminal environments like docker containers,
// which have very few variables. Also, it's very much in the realm of UB.
const char process_names[][9] = {
    "ps",  "top", "htop", "less",   "more",  "grep",  "find",
    "vim", "vi",  "nano", "apache", "mysql", "nginx", "YUROP",
};
