#ifndef INCLUDE_FLEXIBITY_SPAWNUTIL_HPP_
#define INCLUDE_FLEXIBITY_SPAWNUTIL_HPP_

#include <flexibity/log.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <tuple>

extern char **environ;

namespace Flexibity {
	namespace spawnUtil {


		std::tuple<int, pid_t> run_cmd(const char *const argv[],  bool wait = false)
		{
			pid_t pid;
			int status;
			const char *cmd = argv[0];
			GINFO("Run command: " << cmd);
			status = posix_spawnp(&pid, cmd, NULL, NULL, (char **)argv, ::environ);
			if (status == 0) {
				GINFO("Child pid: " << pid);
				if (wait) {
					 if (waitpid(pid, &status, 0) != -1) {
						 GINFO("Child exited with status " << status);
					 } else {
						 GERROR("waitpid");
					 }
				}
			} else {
				GERROR("posix_spawn: " << strerror(status));
			}

			return std::make_tuple(WEXITSTATUS(status), pid);
		}

		std::tuple<int, pid_t> mpg123(std::string cmd)
		{
			const char *const argv[] = {"mpg123", "-a", "dmix", cmd.c_str(), NULL};
			return run_cmd(argv);
		}
	}

}

#endif // INCLUDE_FLEXIBITY_SPAWNUTIL_HPP_
