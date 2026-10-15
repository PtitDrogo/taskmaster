#include <grp.h>
#include <pwd.h>
#include <server.hpp>
#include <sys/types.h>

int static setRightsToUser(const passwd &pw) {
    if (initgroups(pw.pw_name, pw.pw_gid) != 0) {
        perror("initgroups");
        return -1;
    }

    if (setgid(pw.pw_gid) != 0) {
        perror("setgid");
        return -1;
    }

    if (setuid(pw.pw_uid) != 0) {
        perror("setuid");
        return -1;
    }

    if (pw.pw_uid != 0 && setuid(0) == 0) {
        std::cerr << "Fatal: privilege de-escalation failed, still able to regain root\n";
        return -1;
    }
    return 0;
}

int dropPrivilegesServer(const std::string &user) {
    bool userIsSet = !user.empty();
    bool isRoot = geteuid() == 0;
    if (userIsSet) {
        struct passwd *pw = getpwnam(user.c_str());
        if (!pw) {
            std::cerr << "User " << user << " not found\n";
            return -1;
        }

        // already that user, nothing to do
        if (geteuid() == pw->pw_uid) {
            return 0;
        }

        if (setRightsToUser(*pw) == -1) {
            std::cerr << "Fatal: can't drop privileges as non root user" << std::endl;
            return -1;
        }
    }

    if (!userIsSet && isRoot) {
        Logger::get().log(Logger::WARN, "Running as root, no user specified");
    }
    // if not Root we do nothing its chill.
    return 0;
}

int dropPrivilegesProgram(const std::string &user) {
    bool userIsSet = !user.empty();
    if (!userIsSet) {
        // Program runs with rights of parent.
        return 0;
    }

    struct passwd *pw = getpwnam(user.c_str());
    if (!pw) {
        std::cerr << "User " << user << " not found\n";
        return -1;
    }
    if (geteuid() == pw->pw_uid) {
        return 0;
    }

    if (setRightsToUser(*pw) == -1) {
        std::cerr << "Fatal: can't drop privileges as non root user" << std::endl;
        return -1;
    }

    return 0;
}