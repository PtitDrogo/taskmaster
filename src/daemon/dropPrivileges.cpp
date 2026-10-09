#include <grp.h>
#include <pwd.h>
#include <server.hpp>
#include <sys/types.h>

// The main goal is to be able to set children to a given level of rights
// If user=tfreydie is set, we switch the rights to that guy
// If no user is set, we

//-1 on failure
static int dropPrivilegesToUser(const std::string &user) {
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

/*
Is [supervisord] user= set?
├─ No
│   └─ Am I root?
│       ├─ Yes → stay root, log a CRITICAL warning ("running as root, no user specified")
│       └─ No  → nothing to do
└─ Yes
    └─ Is it a valid user?
        ├─ No → exit with usage error
        └─ Yes → Am I already that user?
            ├─ Yes → nothing to do
            └─ No → Am I root?
                ├─ No  → exit with error ("can't drop privileges as non-root")
                └─ Yes → set groups, setgid, setuid → log "Set uid to user X"
*/

int dropPrivilegesServer(const std::string &user) {
    bool userIsSet = !user.empty();
    bool isRoot = geteuid() == 0;
    if (userIsSet) {
        struct passwd *pw = getpwnam(user.c_str());
        if (!pw) {
            std::cerr << "User " << user << " not found\n";
            return -1;
        }

        if (geteuid() == pw->pw_uid) {
            // already that user, nothing to do
            return 0;
        }

        if (isRoot) {
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
        } else {
            std::cerr << "Fatal: can't drop privileges as non root, please re-run with root" << std::endl;
            return -1;
        }
        // how to check if i am that user
    }

    if (!userIsSet && isRoot) {
        std::cout << "CRITICAL WARNING: Running as root, no user specified" << std::endl;
        // if not Root we do nothing its chill.
    }
}

int dropPrivilegesProgram() {}