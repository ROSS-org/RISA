#include "damaris/data/VariableManager.hpp"
#include "damaris/data/Variable.hpp"
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>

#include <plugins/streaming/stream-client.h>
#include <plugins/util/fb-util.h>
#include <plugins/util/damaris-util.h>
#include <plugins/util/sim-config.h>

using namespace ross_damaris;
using namespace ross_damaris::sample;
using namespace ross_damaris::streaming;
using namespace ross_damaris::util;


