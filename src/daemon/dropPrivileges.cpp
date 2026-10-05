#include <grp.h>
#include <pwd.h>
#include <server.hpp>
#include <sys/types.h>

//-1 on failure
int dropPrivileges(const std::string &user) {
    if (geteuid() != 0)
        return 1; // not root, nothing to do

    struct passwd *pw = getpwnam(user.c_str());
    if (!pw) {
        // std::cerr << "User " << user << " not found\n";
        return -1;
    }

    if (initgroups(pw->pw_name, pw->pw_gid) != 0) {
        perror("initgroups");
        return -1;
    }

    if (setgid(pw->pw_gid) != 0) {
        perror("setgid");
        return -1;
    }

    if (setuid(pw->pw_uid) != 0) {
        perror("setuid");
        return -1;
    }

    if (setuid(0) == 0) {
        std::cerr << "Fatal: privilege de-escalation failed, still able to regain root\n";
        return -1;
    }
    std::cout << "Privileges dropped, gg fuggin ez" << std::endl;
    return 1;
}