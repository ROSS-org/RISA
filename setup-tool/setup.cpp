#include "Model.hxx"
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <utility>

using namespace std;
using namespace damaris::model;

void parse_idl(char *file, map<string, vector<pair<string, string>>>& lp_types_map,
        unordered_set<string>& param_types);
string create_new_layout(const string& str, Data::layout_sequence& ls);


// currently run with ./setup <path/to/xml-template> <path/to/idl-file>
int main(int argc, char* argv[])
{
    char *xml_file = argv[1];
    
    // first parse the IDL file to get lp types and variables
    map<string, vector<pair<string, string>>> lp_types_map;
    unordered_set<string> param_types;
    cout << "Parsing IDL file...";
    parse_idl(argv[2], lp_types_map, param_types);
    cout << "complete!\nSetting up XML file...";

    try
    {
        auto_ptr<Simulation> sim (simulation(xml_file, xml_schema::flags::dont_validate));

        // create model group
        Group model_grp("model");
        Group::group_sequence& model_gs = model_grp.group();

        // get pointer to layout_sequences in Data, just in case
        Data::layout_sequence& data_ls = sim->data().layout();

        for (auto& lp_type: lp_types_map)
        {
            // create LP type groups
            Group lp_grp(lp_type.first);
            Group::variable_sequence& group_vs = lp_grp.variable();
            for (auto it = lp_type.second.begin(); it != lp_type.second.end(); ++it)
            {
                string var_type;
                if ((*it).second.find("Param") == 0)
                {
                    // need to create a special layout for this variable
                    var_type = create_new_layout((*it).second, data_ls);
                }
                else
                    var_type = (*it).second;
                // create variables for each LP type
                Variable v((*it).first, var_type);
                //cout << "variable " << v.name() << " layout " << v.layout() << endl;
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
    cout << "complete!\n";
}

// create new layout for model variables if it doesn't already exist
// returns layout name to use as variable type
string create_new_layout(const string& str, Data::layout_sequence& ls)
{
    // expecting string in format Param_name_type
    // where name is the name for the damaris parameter and type is a supported data type
    string layout_name, dim, var_type;

    stringstream ss(str);
    ss.ignore(6, '_');
    stringbuf sb;
    ss.get(sb, '_');
    dim = sb.str();
    ss.ignore(1, '_');
    ss >> var_type;

    // now create layout name
    layout_name = dim + "_" + var_type + "_layout";

    // before we add, make sure it doesn't already exist first
    bool found = false;
    for (auto it = ls.begin(); it != ls.end(); ++it)
    {
        if ((*it).name().compare(layout_name) == 0)
            found = true;
    }
    if (!found)
    {
        Layout l(layout_name, var_type, dim);
        ls.push_back(l);
    }

    return layout_name;
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

pair<string, string> get_var_name_type(const string& str)
{
    string var_name;
    string var_type;

    // first remove any leading spaces
    stringstream ss(str);
    while (ss.peek() == ' ')
        ss.ignore(1, ' ');

    stringbuf name;
    ss.get(name, ':');
    var_name = name.str();

    while (ss.peek() == ' ' || ss.peek() == ':')
        ss.ignore(1, ss.peek());

    stringbuf t;
    if (ss.peek() == '[')
    {
        ss.ignore(1, ss.peek());
        ss.get(t, ']');
    }
    else
        ss.get(t, ';');
    var_type = t.str();
    //cout << "var_name: " << var_name << " var_type: " << var_type << endl;
    pair<string, string> p(var_name, var_type);
    return p;
}

void parse_idl(char *file, map<string, vector<pair<string, string>>>& lp_types_map,
        unordered_set<string>& param_types)
{
    ifstream ifs(file);

    int max_line_size = 512;
    char l[max_line_size];
    ifs.getline(&l[0], max_line_size);
    bool table_parsing = false;
    string cur_lp_type;
    while (ifs.good())
    {
        string line = l;
        if (!table_parsing)
        {
            if (line.find("table") == 0 && line.find("table ModelData") != 0)
            { // this lets us find all user created tables for LP types
                table_parsing = true;
                cur_lp_type = get_table_name(line);
                //cout << "parsing table for lp " << cur_lp_type << endl;
            }
            else if (line.find("struct Param") == 0)
            {
                string custom_type = get_table_name(line);
                param_types.insert(custom_type);
                //cout << "found custom type " << custom_type << endl;
            }
        }
        else
        {
            if (line.find("}") == 0)
            {
                table_parsing = false;
                ifs.getline(&l[0], max_line_size);
                continue;
            }
            auto cur_var = get_var_name_type(line);
            //cout << cur_lp_type << ": " << cur_var.first << " " << cur_var.second << endl;
            lp_types_map[cur_lp_type].push_back(cur_var);

            // in case the '}' isn't on a new line
            if (line.find("}") != string::npos)
                table_parsing = false;
        }

        ifs.getline(&l[0], max_line_size);
    }

    ifs.close();
}
