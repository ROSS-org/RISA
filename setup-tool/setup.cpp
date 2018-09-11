#include "Model.hxx"
#include <iostream>
#include <fstream>
#include <unordered_set>

using namespace std;
using namespace damaris::model;

void parse_idl(char *file, map<string, unordered_set<string>>& lp_types_map,
        map<string, unordered_set<string>>& union_map);

int main(int argc, char* argv[])
{
    char *xml_file = argv[1];
    
    // first parse the IDL file to get lp types and variables
    map<string, unordered_set<string>> lp_types_map;
    map<string, unordered_set<string>> union_map;
    parse_idl(argv[2], lp_types_map, union_map);

    try
    {
        auto_ptr<Simulation> sim (simulation(xml_file, xml_schema::flags::dont_validate));

        // create model group
        Group model_grp("model");
        Group::group_sequence& model_gs = model_grp.group();

        for (auto& lp_type: lp_types_map)
        {
            cout << lp_type.first << endl;
            // create LP type groups
            Group lp_grp(lp_type.first);
            Group::variable_sequence& group_vs = lp_grp.variable();
            for (auto v_it = lp_type.second.begin(); v_it != lp_type.second.end(); ++v_it)
            {
                cout << *v_it << endl;
                // create variables for each LP type
                Variable v(*v_it, "int");
                cout << "variable " << v.name() << " layout " << v.layout() << endl;
                group_vs.push_back(v);
            }
            // need to add all variables before putting this group into the model's group_sequence
            model_gs.push_back(lp_grp);
        }

        // get a ptr to the group_sequence at the Data level of the XML file
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

string get_table_name(const string& str)
{
    // want to grab just the table name from the given string
    stringstream ss(str);
    ss.ignore(128, ' ');
    string name;
    ss >> name;
    return name;
}

string get_var_name(const string& str, char delim)
{
    stringstream ss(str);
    while (ss.peek() == ' ')
        ss.ignore(1, ' ');
    stringbuf name;
    ss.get(name, delim);
    return name.str();
}

void parse_idl(char *file, map<string, unordered_set<string>>& lp_types_map,
        map<string, unordered_set<string>>& union_map)
{
    ifstream ifs(file);

    int max_line_size = 512;
    char l[max_line_size];
    ifs.getline(&l[0], max_line_size);
    bool table_parsing = false;
    bool union_parsing = false;
    string cur_lp_type;
    string cur_union;
    while (ifs.good())
    {
        string line = l;
        if (!table_parsing && !union_parsing)
        {
            if (line.find("table") == 0 && line.find("table ModelData") != 0 &&
                    line.find("table Param") != 0)
            { // this lets us find all user created tables for LP types
                table_parsing = true;
                cur_lp_type = get_table_name(line);
            }
            else if (line.find("table Param") == 0)
            {
                // this table represents a Damaris parameter
            }
            else if (line.find("union") == 0)
            {
                union_parsing = true;
                cur_union = get_table_name(line);
            }
        }
        else if (table_parsing && !union_parsing)
        {
            if (line.find("}") == 0)
            {
                table_parsing = false;
                ifs.getline(&l[0], max_line_size);
                continue;
            }
            string cur_var = get_var_name(line, ':');
            cout << cur_lp_type << ": " << cur_var << endl;
            lp_types_map[cur_lp_type].insert(cur_var);

            // in case the '}' isn't on a new line
            if (line.find("}") != string::npos)
                table_parsing = false;
        }
        else if (union_parsing && !table_parsing)
        {
            if (line.find("}") == 0)
            {
                union_parsing = false;
                ifs.getline(&l[0], max_line_size);
                continue;
            }
            string cur_type = get_var_name(line, ',');
            cout << cur_union << ": " << cur_type << endl;
            union_map[cur_union].insert(cur_type);

            if (line.find("}") != string::npos)
                union_parsing = false;
        }
        else
        {
            cout << "We shouldn't be union_parsing and table_parsing!\n";
        }

        ifs.getline(&l[0], max_line_size);
    }

    ifs.close();
}
