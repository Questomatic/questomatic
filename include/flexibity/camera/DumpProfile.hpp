#ifndef DUMBPROFILE_H_
#define DUMBPROFILE_H_

#include <sys/time.h>
#include <stdint.h>
#include <string.h>
#define DPROFILE dumb_profile dprf = dumb_profile(__FUNCTION__, __LINE__); dprf.start();

struct dumb_profile{

	struct timeval start_;
	struct timeval end_;

	const char* fname_;
	const uint32_t lnum_;

	dumb_profile(const char* fname, const uint32_t lnum): start_(), end_(), fname_(fname), lnum_(lnum){
			//start();
	}

	~dumb_profile(){
		stop();
	}

	void start(){
		gettimeofday(&start_, 0);
	}

	void stop(){
		print();

		memset(&start_, 0, sizeof(start_));
		memset(&end_, 0, sizeof(end_));
	}

	int64_t started(){
		return timeval_to_ms(start_);
	}

	int64_t timeval_to_ms(struct timeval& tv){
		return (tv.tv_sec * 1000000) + tv.tv_usec;
	}

	void print(){
		gettimeofday(&end_, 0);

		//uint32_t now_ms = end_.;
		int32_t delta = ((end_.tv_sec - start_.tv_sec) * 1000000) + end_.tv_usec - start_.tv_usec;
//		printf("profile: %s:%d: now:%03li:04%li elapsed:%d us\n", fname_, lnum_, end_.tv_sec, end_.tv_usec / 1000 , delta);
	}
};

#endif //DUMBPROFILE_H_
