/*
 * programOptions.hpp
 *
 *  Created on: Aug 12, 2015
 *      Author: Roman Savrulin <romeo.deepmind@gmail.com>
 */

#ifndef INCLUDE_FLEXIBITY_PROGRAMOPTIONS_HPP_
#define INCLUDE_FLEXIBITY_PROGRAMOPTIONS_HPP_

#include "flexibity/log.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>


namespace Flexibity {

namespace po = boost::program_options;

class programOptions{
public:
	programOptions():desc("General options"){
		desc.add_options()("help,h", "Show help");
	}
	virtual ~programOptions(){}
	virtual void parse(int ac, char** av) {
		po::parsed_options parsed =
				po::command_line_parser(ac, av).options(desc).allow_unregistered().run();
		po::store(parsed, vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			exit(1);
		}
	}
	po::options_description desc;
	po::variables_map vm;
};

}

#endif /* INCLUDE_FLEXIBITY_PROGRAMOPTIONS_HPP_ */
