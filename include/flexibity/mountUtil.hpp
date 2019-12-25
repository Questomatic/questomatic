#ifndef INCLUDE_FLEXIBITY_MOUNTUTIL_HPP_
#define INCLUDE_FLEXIBITY_MOUNTUTIL_HPP_

#include <flexibity/log.h>
#include <sys/mount.h>

namespace Flexibity {
	namespace mountUtil {
		int safe_mount(const char* fromDev, const char* toPath) {
			int mount_result = ::mount(fromDev, toPath, "ext2", MS_MGC_VAL | MS_RDONLY , "" );
			GWARN("Mount errno: " << "(" << mount_result << ")" << errno << " " << strerror(errno));
			return mount_result;
		}

		int safe_remount(const char* toPath, bool rw) {
			int flags =  MS_REMOUNT | MS_RDONLY;

			if (rw)
				flags = MS_REMOUNT & ~MS_RDONLY;

			int mount_result = ::mount(NULL, toPath, NULL, flags, "" );
			GWARN("Mount errno: " << "(" << mount_result << ")" << errno << " " << strerror(errno));
			return mount_result;
		}

		void mount(const char* fromDev, const char* toPath) {
			int mount_result = safe_mount(fromDev, toPath);
			if(mount_result < 0 ) {

				mount_result = safe_remount(toPath, false);

				if( mount_result < 0 ) {
					system((std::string("mkfs.ext2 ") + std::string(fromDev)).c_str());
					safe_mount(fromDev, toPath);
				}
			}
		}



	}

}

#endif // INCLUDE_FLEXIBITY_MOUNTUTIL_HPP_
