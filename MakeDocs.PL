#!/usr/bin/env perl

use strict;
use warnings;

use File::Spec;
use File::Slurp;
use File::Temp qw(tempdir);

my $SRC_DIR = File::Spec->catdir('docs', 'src');
my $PDF_DIR = File::Spec->catdir('docs', 'pdf');
my $TEX_DIR = File::Spec->catdir('docs', 'tex');

my $ACTION_DIR = File::Spec->catdir('lib', 'Bi', 'Action');
my $BLOCK_DIR = File::Spec->catdir('lib', 'Bi', 'Block');
my $CLIENT_DIR = File::Spec->catdir('lib', 'Bi', 'Client');
my $BUILDER_MODULE = File::Spec->catfile('lib', 'Bi', 'Builder.pm');
my $CLIENT_MODULE = File::Spec->catfile('lib', 'Bi', 'Client.pm');

my $ACTION_TMPDIR = tempdir(CLEANUP => 1);
my $BLOCK_TMPDIR = tempdir(CLEANUP => 1);
my $CLIENT_TMPDIR = tempdir(CLEANUP => 1);

my $ACTION_OUT = File::Spec->catfile($TEX_DIR, 'actions.tex');
my $BLOCK_OUT = File::Spec->catfile($TEX_DIR, 'blocks.tex');
my $CLIENT_OUT = File::Spec->catfile($TEX_DIR, 'clients.tex');
my $BUILD_OUT = File::Spec->catfile($TEX_DIR, 'build.tex');
my $RUN_OUT = File::Spec->catfile($TEX_DIR, 'run.tex');
my $COMMON_OUT = File::Spec->catfile($TEX_DIR, 'common.tex'); 

# make directories
system("mkdir -p $TEX_DIR");
system("mkdir -p $PDF_DIR");

# copy fixed tex files
`cp $SRC_DIR/*.tex $TEX_DIR/.`;

# copy bib files
`cp $SRC_DIR/*.bib $TEX_DIR/.`;

# copy pods to temporary directory
`cp $ACTION_DIR/*.pm $ACTION_TMPDIR`;
`cp $BLOCK_DIR/*.pm $BLOCK_TMPDIR`;
`cp $CLIENT_DIR/*.pm $CLIENT_TMPDIR`;

# preprocess pod
foreach my $dir ($ACTION_TMPDIR, $BLOCK_TMPDIR, $CLIENT_TMPDIR) {
    my @files = <$dir/*.pm>;	
    foreach my $file (@files) {
        my $contents = read_file($file);
        $contents = process_pod($contents);
        write_file($file, $contents);
    }
}

# convert pod to tex
system("pod2latex -h1level 2 -modify -sections 'BUILD OPTIONS' -out $BUILD_OUT $BUILDER_MODULE");
system("pod2latex -h1level 2 -modify -sections 'RUN OPTIONS' -out $RUN_OUT $CLIENT_MODULE");
system("pod2latex -h1level 2 -modify -sections 'COMMON OPTIONS' -out $COMMON_OUT $CLIENT_MODULE");
system("pod2latex -h1level 2 -modify -sections '!(AUTHOR|VERSION|INHERITS|METHODS)' -out $ACTION_OUT `ls $ACTION_TMPDIR/*[A-Za-z0-9].pm | sort`");
system("pod2latex -h1level 2 -modify -sections '!(AUTHOR|VERSION|INHERITS|METHODS)' -out $BLOCK_OUT `ls $BLOCK_TMPDIR/*[A-Za-z0-9].pm | sort`");
system("pod2latex -h1level 2 -modify -sections '!(AUTHOR|VERSION|INHERITS|METHODS)' -out $CLIENT_OUT `ls $CLIENT_TMPDIR/*[A-Za-z0-9].pm | sort`");

# postprocess tex
foreach my $file ($BUILD_OUT, $RUN_OUT, $COMMON_OUT, $ACTION_OUT, $BLOCK_OUT, $CLIENT_OUT) {
    my $contents = read_file($file);
    $file =~ /(\w+)s.tex$/;
    $contents = &process_tex($contents, $1);
    write_file($file, $contents);
}

# convert tex to html
# (broken)
#system("cd $HTML_DIR; latex2html -antialias -antialias_text -show_section_numbers -local_icons -toc_depth 4 -split 5 -dir ../html index.tex");

# compile tex to PDF
system("cd $TEX_DIR; pdflatex index.tex; bibtex index; makeindex index; pdflatex index.tex; pdflatex index.tex");
`cp $TEX_DIR/index.pdf $PDF_DIR/manual.pdf`;

sub process_tex {
    my $contents = shift;
    my $type = shift;

    if (defined $type) {
        $contents =~ s/\\label\{([A-Za-z0-9\-\_\\]*?)\}/\\label\{${type}_$1\}/g;
    }
    $contents =~ s/\\texttt\{/\\bitt\{/g;
    $contents =~ s/\\begin\{verbatim\}((?:.|\n)*?)\\end\{verbatim\}/&process_tt($1)/gem;
    $contents =~ s/I<(.*?)>/\\textsl\{$1\}/g;
    $contents =~ s/(SYNOPSIS|DESCRIPTION|ARGUMENTS|PARAMETERS|OPTIONS|SEE ALSO|BUILD OPTIONS|RUN OPTIONS|COMMON OPTIONS)/ucfirst(lc($1))/ge;
    $contents =~ s/\$\\backslash\$secref\{([A-Za-z0-9\-\_\\]*?)\}\\\{([A-Za-z0-9\-\_\\]*?)\\\}/"\\secref\{" . &process_label($1) . "\}\{$2\}"/ge;
    $contents =~ s/\$\\backslash\$actionref\\\{([A-Za-z0-9\-\_\\]*?)\\\}/"\\actionref\{" . &process_label($1) . "\}"/ge;
    $contents =~ s/\\label\{([A-Za-z0-9\-\_\\]*?)\}/"\\label\{" . &process_label($1) . "\}"/ge;
    $contents =~ s/\\index\{([A-Za-z0-9\-\_\\ ]+?)\}/"\\index\{" . &process_index($1) . "\}"/ge;
    $contents =~ s/\\mbox\{\}//g;
    $contents =~ s/\\item\[\{?(.*?) *(\((?:position ?\d+,? ?)?(?:(?:default ?|mandatory).*?)?\)?)\}?\]/\\item\[$1\] $2/g;
    $contents =~ s/\\textbf\{(\(default .*?\))\}/$1/g; # longer command line options seem to end up with some bold?
    $contents =~ s/\(default ?\\textbf\{ ?(.*?)\}?\)\}?/\(default $1\)/g; # ^ same again
    $contents =~ s/\\subsubsection\*\{See also\\label\{.*?\}\}\n\n\n(\\\w+ref\{.*\})/\\newseealso\{See also $1\}/g;

    # remove index entries for subsections and paragraphs
    #$contents =~ s/\\index\{[A-Za-z0-9\-\_\\ ]+?\!(?:SYNOPSIS|DESCRIPTION|ARGUMENTS|PARAMETERS|OPTIONS|SEE ALSO)\}//ig;
    $contents =~ s/(\\sub(?:sub)?section\*?)\{(.*?)\\index\{.*?\}\}/$1\{$2\}/g;
    $contents =~ s/(\\paragraph\*?\{.*?)\\index\{.*?\}/$1/g;

    $contents =~ s/\\subsection\{((?:\w|\\_)+)\\/\\subsection{\\bitt\{$1\}\\/g;
    
    return $contents;
}

sub process_tt {
    my $contents = shift;
	
    $contents =~ s/([{}])/\\$1/g;
    $contents =~ s/\.\.\./\\(\\ldots\\)/g;
    $contents =~ s/^    //gm;
    
    return "\\begin\{bicode\}$contents\\end\{bicode\}";
}

sub process_label {
    my $contents = shift;
	
    $contents =~ s/\\_/_/g;
    $contents =~ s/-/_/g;
	
    return $contents;
}

sub process_index {
    my $contents = shift;

    $contents =~ s/(?:<\\)_/\\_/g;
    $contents =~ s/-/_/g;
    $contents =~ s/\\ /_/g;
	
    return $contents;
}

sub process_pod {
    my $contents = shift;
	
    $contents =~ s/L\<(\w+)\>/\\actionref\{$1\}/g;
    $contents =~ s/L\<(\w+)\|(\w+)\>/\\secref\{$2\}\{$1\}/g;
	
    return $contents;
}
