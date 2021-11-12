# About Themisto
Themisto is a succinct colored de Bruijn graph supporting pseudo-alignment against a database of reference sequences similar to the tool Kallisto. For more information, see the [webpage](https://www.helsinki.fi/en/researchgroups/genome-scale-algorithmics/themisto) and the [paper](https://www.biorxiv.org/content/biorxiv/early/2020/04/04/2020.04.03.021501/DC1/embed/media-1.pdf?download=true).

Themisto takes as an input a set of sequences in FASTA or FASTQ format, and a file specifying the color (a non-negative integer) of each sequence. The i-th line of the color file contains the color of the i-th sequence in the sequence file. For optimal compression, use color numbers in the range [0, n-1], where n is the number of distinct colors. If no color file is given, the index is built without colors. This way, the user can later try multiple colorings without recomputing the de Bruijn graph.

The de Bruijn graph is defined so that the nodes represent k-mers and edges (k+1)-mers. There is an edge from u to v if there is a (k+1)-mer in the data that is suffixed by u and prefixed by v. The set of nodes is the set of endpoints of all edges. Note that this implies that orphan k-mers (those that are not connected to any edge) are not in the graph.

We use the KMC3 library to list the distinct (k+1)-mers to construct the graph. Since KMC3 only works with the DNA alphabet ACGT, we must preprocess the data so that it does not have any characters outside of the alphabet ACGT. The default behavior is the replace those characters with random nucleotides. If instead you would like to *delete* all k-mers that are not from the alphabet ACGT, pass in the option `--delete-non-ACGT`. This effectively removes all nodes and edges that would contain a character outside of the alphabet ACGT. If you would like to deal with non-ACGT characters differently, please preprocess the data yourself.

# Installation
## Requirements
Compilation: C++17 compliant compiler with OpenMP support, and CMake v3.1 or newer. If compiling with g++, make sure that the version is at least g++-8, or you might run into compilation errors with the standard library &lt;filesystem&gt; header.

## Compiling
A c++-17-compliant compiler is required. Enter the Themisto directory and run

```
cd build
cmake .. -DMAX_KMER_LENGTH=60
make
```

Where 60 is the maximum k-mer length to support, up to 255. The larger the k-mer length, the more time and memory the index construction takes.

This will produce the build\_index, pseudoalignment, and
themisto\_tests executables in the build/bin/ directory.

### Troubleshooting

If you run into problems involving zlib or bzip2, you can instruct the
build process to download & compile them from source with

```
cmake -DCMAKE_BUILD_ZLIB=1 -DCMAKE_BUILD_BZIP2=1 ..
make
```

If you run into problems involving the &lt;filesystem&gt; header, you probably need to update your compiler. The compiler `g++-8` should be sufficient. Install a new compiler and direct CMake to use it with the `-DCMAKE_CXX_COMPILER` option. For example, to set the compiler to `g++-8`, run CMake with the option `-DCMAKE_CXX_COMPILER=g++-8`. 

## Compiling on macOS
Compiling Themisto on macOS requires users to first install gcc-8 or
newer from homebrew with

```
brew install gcc@8
```

Afterwards, Themisto can be compiled by entering the directory and running

```
cd build
cmake -DCMAKE_C_COMPILER=$(which gcc-8) -DCMAKE_CXX_COMPILER=$(which g++-8) -DMAX_KMER_LENGTH=60 ..
make
```

Where 60 is the maximum k-mer length to support, up to 255. The larger the k-mer length, the more time and memory the index construction takes. Note that macOS has a very small limit for the number of concurrently
opened files. Themisto can use temporary files to conserve RAM, and
may run into this limit. To increase the limit, run the command

```
ulimit -n 2048
```

# Usage
## Indexing

### Quick start

There is an example dataset at example_output. To build the index with order k = 30, such that the index files are written to directory `example_index`, using the directory `temp`as temporary storage, using four threads and up to 1GB of memory, deleting non-ACGT-characters, run the following:

```
./build/bin/build_index --node-length 30 -i example_input/coli3.fna -c example_input/colors.txt -o example_index --temp-dir temp --mem-megas 1000 -t 4 --delete-non-ACGT
```

We recommend to use a fast SSD drive for the temporary directory. With a reasonable desktop workstation and an SSD drive, the program should take about one minute on this example input.

### Full instructions for `build_index`

This program builds an index consisting of compact de Bruijn graph using the BOSS data structure (implemeted as a [Wheeler graph](https://www.sciencedirect.com/science/article/pii/S0304397517305285)) and color information. The input is a set of reference sequences in a single file in fasta or fastq format, and a colorfile, which is a plain text file containing the colors of the reference sequences in the same order as they appear in the reference sequence file, one line per sequence. The names are given as ASCII strings, but they should not contain whitespace characters. If there are characters outside of the DNA alphabet ACGT in the input sequences, those are replaced with random characters from the DNA alphabet.

```
Usage:
  ./build/bin/build_index [OPTION...]

      --load-boss               If given, loads a precomputed BOSS from the 
                                index directory
  -k, --node-length arg         The k of the k-mers. Required only if 
                                --load-boss is not given
  -i, --input-file arg          The input sequences in FASTA or FASTQ 
                                format. The format is inferred from the 
                                file extension. Recognized file extensions 
                                for fasta are: .fasta, .fna, .ffn, .faa and 
                                .frn . Recognized extensions for fastq are: 
                                .fastq and .fq . If the file ends with .gz, 
                                it is uncompressed into a temporary 
                                directory and the temporary file is deleted 
                                after use.
  -c, --color-file arg          One color per sequence in the fasta file, 
                                one color name per line. Required only if 
                                you want to build the colors. (default: "")
      --auto-colors             Instead of a color file let the program 
                                automatically give colors integer names 
                                (0,1,2...)
  -o, --index-dir arg           Directory where the index will be built. 
                                Always required, directory must exist 
                                before running.
  -d, --colorset-pointer-tradeoff arg
                                This option controls a time-space tradeoff 
                                for storing and querying color sets. If 
                                given a value d, we store color set 
                                pointers only for every d nodes on every 
                                unitig. The higher the value of d, the 
                                smaller then index, but the slower the 
                                queries. The savings might be significant 
                                if the number of distinct color sets is 
                                small and the graph is large and has long 
                                unitigs. (default: 1)
      --temp-dir arg            Directory for temporary files.
  -m, --mem-megas arg           Number of megabytes allowed for external 
                                memory algorithms. Default: 1000 (default: 
                                1000)
  -t, --n-threads arg           Number of parallel exectuion threads. 
                                Default: 1 (default: 1)
      --delete-non-ACGT         Delete k-mers that have a letter outside of 
                                the DNA alphabet ACGT. If this option is 
                                not given, the non-ACGT letters are 
                                replaced with random nucleotides.
      --pp-buf-siz arg          Size of preprocessing buffer (in bytes) for 
                                fixing alphabet (default: 4096)
  -h, --help                    Print usage

```

## Pseudoalignment

### Examples

Pseudoalign reads.fna against an index:
```
./build/bin/pseudoalign --query-file reads.fna --index-dir index --temp-dir temp --out-file out.txt
```

Pseudoalign a list of fasta files in input_list.txt into output filenames in output_list.txt:
```
./build/bin/pseudoalign --query-file-list input_list.txt --index-dir index --temp-dir temp --out-file-list output_list.txt
```

Pseudoalign reads.fna against an index using also reverse complements:
```
./build/bin/pseudoalign --rc --query-file reads.fna --index-dir index --temp-dir temp --outfile out.txt
```

### Full instructions for `pseudoalign`

This program aligns query sequences against an index that has been built previously. The output is one line per input read. Each line consists of a space-separated list of integers. The first integer specifies the rank of the read in the input file, and the rest of the integers are the identifiers of the colors of the sequences that the read pseudoaligns with. If the program is ran with more than one thread, the output lines are not necessarily in the same order as the reads in the input file. This can be fixed with the option --sort-output.

The query can be given as one file, or as a file with a list of files. In the former case, we must specify one output file with the options --out-file, and in the latter case, we must give a file that lists one output filename per line using the option --out-file-list.

The query file(s) should be in fasta of fastq format. The format is inferred from the file extension. Recognized file extensions for fasta are: .fasta, .fna, .ffn, .faa and .frn . Recognized extensions for fastq are: .fastq and .fq

```
Usage:
  ./build/bin/pseudoalign [OPTION...]

  -q, --query-file arg       Input file of the query sequences (default: )
      --query-file-list arg  A list of query filenames, one line per filename
                             (default: )
  -o, --out-file arg         Output filename. (default: )
      --out-file-list arg    A file containing a list of output filenames,
                             one per line. (default: )
  -i, --index-dir arg        Directory where the index will be built. Always
                             required, directory must exist before running.
      --temp-dir arg         Temporary directory. Always required, directory
                             must exist before running.
      --rc                   Whether to to consider the reverse complement
                             k-mers in the pseudoalignemt.
  -t, --n-threads arg        Number of parallel exectuion threads. Default: 1
                             (default: 1)
      --gzip-output          Compress the output files with gzip.
      --sort-output          Sort the lines of the out files by sequence rank
                             in the input files.
  -h, --help                 Print usage
```

## Extracting unitigs with `extract_unitigs`

This program dumps the unitigs and optionally their colors out of an existing Themisto index.

```
  ./build/bin/extract_unitigs [OPTION...]

  -i, --index-dir arg    Location of the Themisto index.
      --unitigs-out arg  Output filename for the unitigs (outputted in 
                         FASTA format). (default: "")
      --colors-out arg   Output filename for the unitig colors. If this 
                         option is not given, the colors are not computed. 
                         Note that giving this option affects the unitigs 
                         written to unitigs-out: if a unitig has nodes with 
                         different color sets, the unitig is split into 
                         maximal segments of nodes that have equal color 
                         sets. The file format of the color file is as 
                         follows: there is one line for each unitig. The 
                         lines contain space-separated strings. The first 
                         string on a line is the FASTA header of a unitig, 
                         and the following strings on the line are the 
                         integer color labels of the colors of that unitig. 
                         The unitigs appear in the same order as in the 
                         FASTA file. (default: "")
  -h, --help             Print usage


```

# For developers: building the tests

```
cd googletest
mkdir build
cd build
cmake ..
make
cd ../../build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DMAX_KMER_LENGTH=255
make
```

This builds the tests to build/bin/themisto_tests

# License

This software is licensed under GPLv2. See LICENSE.txt.
