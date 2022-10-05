#include "globals.hh"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <iterator>
#include "sbwt/buffered_streams.hh"
#include "sbwt/SeqIO.hh"

typedef long long LL;

void create_directory_if_does_not_exist(string path){
    std::filesystem::create_directory(path);
}

// https://stackoverflow.com/questions/18100097/portable-way-to-check-if-directory-exists-windows-linux-c
void check_dir_exists(string path){
    struct stat info;    
    if( stat( path.c_str(), &info ) != 0 ){
        cerr << "Error: can not access directory " << path << endl;
        exit(1);
    }
    else if( info.st_mode & S_IFDIR ){
        // All good
    }    
    else{
        cerr << "Error: is not a directory: " << path << endl;
        exit(1);
    }
}

// Returns filename of a new color file that has one color for each sequence
// Input format is either "fasta" or "fastq"
string generate_default_colorfile(string inputfile, string file_format){
    string colorfile = sbwt::get_temp_file_manager().create_filename();
    sbwt::Buffered_ofstream<> out(colorfile);
    sbwt::SeqIO::Reader<> sr(inputfile);
    stringstream ss;
    int64_t seq_id = 0;
    while(true){
        int64_t len = sr.get_next_read_to_buffer();
        if(len == 0) break;
        ss.str(""); ss << seq_id << "\n";
        out.write(ss.str().data(), ss.str().size());
        seq_id++;
    }
    return colorfile;
}


// We need this function because the standard library stoll function accepts all kinds of crap,
// such as "123aasfjhk" and "1 2 3 4" as a number. This function check that the string is a valid
// number and returns that number, or throws an error otherwise.
LL string_to_integer_safe(const string& S){

    // Figure out leading and trailing whitespace
    LL pos_of_first_digit = 1e18;
    LL pos_of_last_digit = -1;
    for(LL i = 0; i < (LL)S.size(); i++){
        if(!std::isdigit(S[i]) && !std::isspace(S[i]))
            throw std::runtime_error("Error parsing color file: could not parse integer: " + S);
        if(std::isdigit(S[i])){
            pos_of_first_digit = min(pos_of_first_digit, i);
            pos_of_last_digit = max(pos_of_last_digit, i);
        }
    }
    if(pos_of_last_digit == -1) throw std::runtime_error("Error parsing color file: could not parse integer: " + S); // No digits found

    // Check that there are no internal spaces
    for(LL i = pos_of_first_digit; i <= pos_of_last_digit; i++)
        if(!std::isdigit(S[i])) throw std::runtime_error("Error parsing color file: could not parse integer: " + S); // Internat whitespace

    // Checks ok, convert to integer
    return stoll(S);
}

vector<int64_t> read_colorfile(string filename){
    vector<int64_t> seq_to_color;
    sbwt::Buffered_ifstream<> colors_in(filename);
    string line;
    while(colors_in.getline(line)){
        seq_to_color.push_back(string_to_integer_safe(line));
    }
    return seq_to_color;
}


// Returns new inputfile and new colorfile
// If colorfile == "", the returned colorfile is also ""
// Input file can't be ""
pair<string,string> split_all_seqs_at_non_ACGT(string inputfile, string inputfile_format, string colorfile){
    if(inputfile == "") throw std::runtime_error("Empty input file");
    if(inputfile_format != "fasta" && inputfile_format != "fastq") throw std::runtime_error("Unkown input format" + inputfile_format);

    string new_colorfile = (colorfile == "" ? "" : sbwt::get_temp_file_manager().create_filename());
    string new_seqfile = sbwt::get_temp_file_manager().create_filename("",".fna");

    vector<int64_t> colors;
    sbwt::Buffered_ofstream<> colors_out;

    if(colorfile != "") {
        colors = read_colorfile(colorfile);
        colors_out.open(new_colorfile);
    }

    sbwt::Buffered_ofstream<> sequences_out(new_seqfile);

    sbwt::SeqIO::Reader<> sr(inputfile);
    LL seq_id = 0;
    LL n_written = 0;
    stringstream ss;
    while(true){
        LL len = sr.get_next_read_to_buffer();
        if(len == 0) break;

        // Chop the sequence into pieces that have only ACGT characters
        sr.read_buf[len] = '$'; // Trick to avoid having a special case for the last sequence. Replaces null-terminator
        string new_seq;
        for(LL i = 0; i <= len; i++){
            char c = sr.read_buf[i];
            if((!(c >= 'A' && c <= 'Z') && c != '$'))
                throw runtime_error("Invalid character found: '" + std::string(1,c) + "'");

            if(c == 'A' || c == 'C' || c == 'G' || c == 'T')
                new_seq += c;
            else{
                if(new_seq.size() > 0){
                    ss.str(""); ss << ">\n" << new_seq << "\n";
                    sequences_out.write(ss.str().data(), ss.str().size());
                    if(colorfile != ""){
                        ss.str(""); ss << colors[seq_id] << "\n";
                        colors_out.write(ss.str().data(), ss.str().size());
                    }
                    new_seq = "";
                    n_written++;
                }
            }
        }
        seq_id++;
    }

    if(n_written == 0) throw std::runtime_error("Error: no (k+1)-mers left after deleting non-ACGT-characters");

    return {new_seqfile, new_colorfile};
}

static constexpr char R_conv_tbl[] = { 'A', 'G' };
static constexpr char Y_conv_tbl[] = { 'C', 'T' };
static constexpr char K_conv_tbl[] = { 'G', 'T' };
static constexpr char M_conv_tbl[] = { 'A', 'C' };
static constexpr char S_conv_tbl[] = { 'C', 'G' };
static constexpr char W_conv_tbl[] = { 'A', 'T' };
static constexpr char B_conv_tbl[] = { 'C', 'G', 'T' };
static constexpr char D_conv_tbl[] = { 'A', 'G', 'T' };
static constexpr char H_conv_tbl[] = { 'A', 'C', 'T' };
static constexpr char V_conv_tbl[] = { 'A', 'C', 'G' };
static constexpr char N_conv_tbl[] = { 'A', 'C', 'G', 'T' };

char fix_char(char c){
	c = toupper(c);
    int rd = std::rand();
    
	switch (c) {
	case 'A':
		return c;
	case 'C':
		return c;
	case 'G':
		return c;
	case 'T':
		return c;
	case 'U':
		return 'T';
	case 'R':
		return R_conv_tbl[rd % 2];
	case 'Y':
		return Y_conv_tbl[rd % 2];
	case 'K':
		return K_conv_tbl[rd % 2];
	case 'M':
		return M_conv_tbl[rd % 2];
	case 'S':
		return S_conv_tbl[rd % 2];
	case 'W':
		return W_conv_tbl[rd % 2];
	case 'B':
		return B_conv_tbl[rd % 3];
	case 'D':
		return D_conv_tbl[rd % 3];
	case 'H':
		return H_conv_tbl[rd % 3];
	case 'V':
		return V_conv_tbl[rd % 3];
	default:
		return N_conv_tbl[rd % 4];
	}
}

// Makes a copy of the file and replaces bad characters. Returns the new filename
// The new file is in fasta format
std::string fix_alphabet(const std::string& input_file){
    const std::string output_file = sbwt::get_temp_file_manager().create_filename("seqs-",".fna");
    sbwt::Buffered_ofstream<> out(output_file);

    LL n_replaced = 0;
    sbwt::SeqIO::Reader<> sr(input_file);
    string header = ">\n";
    string newline = "\n";
    while(true){
        LL len = sr.get_next_read_to_buffer();
        if(len == 0) break;
        for(LL i = 0; i < len; i++) {
            char c = fix_char(sr.read_buf[i]);
            if(c != sr.read_buf[i]) n_replaced++;
            sr.read_buf[i] = c;
        }
        out.write(header.data(), 2);
        out.write(sr.read_buf, len);
        out.write(newline.data(), 1);
    }
    
    sbwt::write_log("Replaced " + to_string(n_replaced) + " characters", sbwt::LogLevel::MAJOR);
    
    return output_file;
}

// true if S is colexicographically-smaller than T
bool colex_compare(const string& S, const string& T){
    LL i = 0;
    while(true){
        if(i == S.size() || i == T.size()){
            // One of the strings is a suffix of the other. Return the shorter.
            if(S.size() < T.size()) return true;
            else return false;
        }
        if(S[S.size()-1-i] < T[T.size()-1-i]) return true;
        if(S[S.size()-1-i] > T[T.size()-1-i]) return false;
        i++;
    }
}


// Split by whitespace
vector<string> split(string text){
    std::istringstream iss(text);
    std::vector<std::string> results(std::istream_iterator<std::string>{iss},
                                 std::istream_iterator<std::string>());
    return results;
}

// Split by delimiter
vector<string> split(string text, char delimiter){
    assert(text.size() != 0); // If called with empty string we probably have a bug
    vector<LL> I; // Delimiter indices
    I.push_back(-1);
    for(LL i = 0; i < text.size(); i++){
        if(text[i] == delimiter){
            I.push_back(i);
        }
    }
    I.push_back(text.size());
    vector<string> tokens;
    for(LL i = 0; i < I.size()-1; i++){
        LL len = I[i+1] - I[i] + 1 - 2;
        tokens.push_back(text.substr(I[i]+1, len));
    }
    
    return tokens;
}

vector<string> split(const char* text, char delimiter){
    return split(string(text), delimiter);
}
