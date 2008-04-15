use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'	=> 'NASTD',
    'VERSION_FROM' => 'NASTD.pm', # finds $VERSION
    'LIBS'	=> ['-L../client -lnast'],   # e.g., '-lm' 
    'DEFINE'	=> '',     # e.g., '-DHAVE_SOMETHING' 
    'INC'	=> '',     # e.g., '-I/usr/include/other' 
    'depend'	=> {'NAST.xs' => '../client/libnast.a'}
);
