# $Id: test.pl,v 1.20 2001/03/23 00:06:09 shmit Exp $
#
# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

######################### We start with some black magic to print on failure.

# Change 1..1 below to 1..last_test_to_print .
# (It may become useful if the test is moved to ./t subdirectory.)

BEGIN { $| = 1; print "1..8\n"; }
END {print "not ok 1\n" unless $loaded;}
use NASTD;
$loaded = 1;
print "ok 1\n";

######################### End of black magic.

# Insert your test code below (better if it prints "ok 13"
# (correspondingly "not ok 13") depending on the success of chunk 13
# of the test code):

$nasthole = &NASTD::nast_sphincter_new();
if (!defined($nasthole)) {
	print "not ok 2\n";
} else {
	print "ok 2\n";
}

#
# Set options
#
@setopts = (1, 1, 0, 0, 0, 1, 5);
if (&NASTD::nast_options_set($nasthole, @setopts) == -1) {
	print "not ok 3\n";
} else {
	print "ok 3\n";
}

#
# Check options
#
@optarr = &NASTD::nast_options_get($nasthole);
$nitems = $#optarr + 1;
for ($i = 0; $i < $nitems; $i++) {
	if ($optarr[$i] != $setopts[$i]) {
		print "not ok 4\n";
		last;
	}
}
if ($i == $nitems) {
	print "ok 4\n";
}

#
# Perform get
#
$rv = &NASTD::nast_get($nasthole, "shmit");
if ($rv == -1) {
	print "not ok 5\n";
} else {
	print "ok 5\n";
}

#
# Perform update
#
@vals = ("foo", "bar", "baz");

$rv = &NASTD::nast_upd($nasthole, "shmit", @vals);
if ($rv == -1) {
	print "not ok 6\n";
} else {
	print "ok 6\n";
}

#
# Verify update
#
$rv = &NASTD::nast_get($nasthole, "shmit");
if ($rv == -1) {
	print "not ok 7\n";
} else {
	print "ok 7\n";
}

$rv = &NASTD::nast_geterr($nasthole);
if ($rv != 0) {
	print "not ok 8\n";
} else {
	print "ok 8\n";
}

#
# Close the sphincter when we don't need it anymore.
#
&NASTD::nast_sphincter_close($nasthole);

#
# This grabs the results and prints them.
#
sub print_results
{
	my @vals = &NASTD::nast_get_result($nasthole);

	my $nitems = $#vals + 1;
	for ($i = 0; $i < $nitems; $i++) {
		my $foo = shift(@vals);
		print "Result[$i]: $foo\n";
	}
}
