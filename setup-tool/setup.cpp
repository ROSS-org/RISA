#include "Model.hxx"
#include <iostream>
#include <fstream>

using namespace std;
using namespace damaris::model;

int main(int argc, char* argv[])
{
    try
    {
        auto_ptr<Simulation> sim (simulation(argv[1], xml_schema::flags::dont_validate));

        // create model group
        Group model_grp("model");

        // get a ptr to the Data level of the XML file
        Data::group_sequence& gs = sim->data().group();

        // add a new group for the model data to be added
        gs.push_back(model_grp);

        // serialize object model to XML
        xml_schema::namespace_infomap map;
        map[""].name = "http://damaris.gforge.inria.fr/damaris/model";

        // output to file
        std::ofstream ofs ("generated.xml");
        simulation(ofs, *sim, map);

    }
    catch (const xml_schema::exception& e)
    {
        cerr << e << endl;
        return 1;
    }
}
