#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool Preprocess(ifstream& input_file, ofstream& output_file, const path& in_file_path, const vector<path>& include_directories){
    static regex include_quotes(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex include_brackets(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    
    int line = 0;
    string str;
    while (getline(input_file, str)){
        ++line;
        if(str.size() == 0){
            output_file << endl;
            continue;
        }
        smatch m1, m2;

        if (!regex_match(str, m1, include_quotes) && !regex_match(str, m2, include_brackets)) {
           output_file << str + "\n"s;
        } else {
            bool file_found = false;
            path p;
            path current_p;
            ifstream input_internal;
            if (regex_match(str, m1, include_quotes)){
                p = string(m1[1]);
                //cout << p.string() << endl;
                current_p = in_file_path.parent_path()/p;
                if (filesystem::exists(current_p)){
                    file_found = true;
                } else {
                    for (const auto& dir : include_directories){
                    current_p =  dir/p;
                    if(filesystem::exists(current_p)){
                        file_found = true;
                        break;
                    }
                    }
                }
            } else{  
                regex_match(str, m2, include_brackets);
                p = string(m2[1]);
                for (const auto& dir : include_directories){
                    current_p =  dir/p;
                    if(filesystem::exists(current_p)){
                        file_found = true;
                       break;
                    }
                }   
            } 
            if (file_found){
                    input_internal.open(current_p);
                    Preprocess(input_internal, output_file, current_p, include_directories);
                } else {
                 std::cout << "unknown include file " << p.string() << " at file " << in_file_path.string() << " at line " << line << endl ;
                    return false;
                }    
        } 
    }
     return true;
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories){
    ifstream input_file(in_file);
    if(!input_file.is_open()){
       cerr << "Input file not found" << std::endl;
        return false;
    }
    
    ofstream output_file(out_file);
    if(!output_file.is_open()){
        cerr << "Output file not found" << std::endl;
        return false;
    }
    
    return Preprocess(input_file, output_file, in_file, include_directories);
}




string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}