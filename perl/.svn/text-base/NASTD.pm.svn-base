# $Id: NASTD.pm,v 1.7 2001/10/29 11:18:20 shmit Exp $

package NASTD;

use strict;
use Carp;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK $AUTOLOAD);

require Exporter;
require DynaLoader;
require AutoLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
	NAST_OK
	NAST_SERVER_GONE
	NAST_NOMEM
	NAST_UNKNOWN_RESPONSE
	NAST_TIMEDOUT
	NAST_UNKNOWN_OPT
	NAST_SERVER_ERR
);
$VERSION = '0.01';

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.  If a constant is not found then control is passed
    # to the AUTOLOAD in AutoLoader.

    my $constname;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    croak "& not defined" if $constname eq 'constant';
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {
	if ($! =~ /Invalid/) {
	    $AutoLoader::AUTOLOAD = $AUTOLOAD;
	    goto &AutoLoader::AUTOLOAD;
	}
	else {
		croak "Your vendor has not defined NASTD macro $constname";
	}
    }
    no strict 'refs';
    *$AUTOLOAD = sub () { $val };
    goto &$AUTOLOAD;
}

bootstrap NASTD $VERSION;

# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__
# Below is the stub of documentation for your module. You better edit it!

=head1 NAME

NASTD - Low level Perl extension for NASTD access methods

=head1 SYNOPSIS

  use NASTD;

  $nasthole = &NASTD::nast_sphincter_new();
  &NASTD::nast_sphincter_close($nasthole);

  $rv = &NASTD::nast_options_get($nasthole);
  $rv = &NASTD::nast_options_set($nasthole, @options);

  @values = &NASTD::nast_get_result($nasthole);

  $rv = &NASTD::nast_add($sphincter, $key);
  $rv = &NASTD::nast_del($sphincter, $key);
  $rv = &NASTD::nast_get($sphincter, $key);
  $rv = &NASTD::nast_upd($sphincter, $key, $values);

  $rv = &NASTD::nast_stats($sphincter);

  $errcode = &NASTD::nast_geterr($sphincter);
  $errstring = &NASTD::nast_errmsg($sphincter);

=head1 DESCRIPTION

The Perl NASTD module allows access to the NASTD server via a Perl
interface. It allows you to do anything you could do through C, such as
get records, update records, or delete records. It also allows setting
of various NASTD connection options, as you would through C.

=head2 Opening a sphincter

To do any work with NASTD at all, you must first open a sphincter,
through which you talk to your NASNASTole. This is fairly
straightforward:  just call C<nast_sphincter_new()> and save the
returned handle. When you're done with the connection to your NAST, call
C<nast_sphincter_close()> to terminate it and free all memory of it.

You may specify an optional argument, which points to the unix domain
socket name that you wish to use. By default, this is set to
"/tmp/nastd.sock".

When an error occurs in C<nast_sphincter_new()>, you cannot call
C<nast_geterr()> since there's no sphincter with which to call it.
Instead, an error message will be printed on B<STDERR> for you to
diagnose, and the return value will be undefined.

=head2 Checking errors

There are many places during an NASTD session that errors can be
generated. To check for the presense of an error, call C<nast_geterr()>
with the sphincter returned from C<nast_sphincter_new()>. This will
return an error code, which is defined below under B<CONSTANTS>.

If you would rather a printable string be returned, you can call
C<nast_errmsg()> with an open sphincter, and a printable string will be
returned.

=head2 Performing a query

Once you have an open sphincter, obtained via C<nast_sphincter_new()>,
you can start to query it with C<nast_add()>, C<nast_del()>,
C<nast_get()>, and C<nast_upd()>.

The B<$key> is just that, a case-sensitive key for the database you're
querying. It's function depends on which query function you're using:

	Function		Key Meaning
	--------		-----------
	nast_add()		Add this key to NASTD, with default values.
	nast_del()		Delete this key from NASTD.
	nast_get()		Return values associated with this key.
	nast_upd()		Update this key in NASTD with my values.

All four of these functions return a status. If everything went okay,
then 0 is returned, otherwise -1 is returned, and you should check for
an error with C<nast_geterr()>.

In the case of C<nast_add()>, C<nast_del()>, and C<nast_upd()> all you
need to do is call the query function and check for errors. If there
are no errors, then everything went fine.

However, in the case of C<nast_get()>, you probably want the values
nastociated with the query you made. To get these values, you have to
call C<nast_get_result()>, which returns an array of values.

=head2 Updating the database via C<nast_upd()>

C<nast_upd()> works like the other query functions, except that in
addition to the key, you also need to pass an array of values. The
array needs to have the same number of elements as the NASTD database
has, and the elements should be in the same order as that specified in
the special key, "B<_VALUES_>" (see B<Key concepts and values>,
below).

If the key you are trying to update does not exist, the server creates
it and gives it the values you specified in your value array. For this
reason C<nast_upd()> is preferable to C<nast_add()>. In fact,
C<nast_add()> may not exist very much longer.

=head2 Key concepts and values

It is recommended that before you try to do any real work with a
particular NASTD suppository, you first investigate the contents of the
special keys B<_KEY_> and B<_VALUES_>. You can do a regular query on
them using C<nast_get($nasthole, "_VALUES_")> followed by
C<nast_get_result()> to investigate their columns.

	Special Key		Value Meaning
	-----------		-------------
	_KEY_			The type of key the database is keyed on,
				e.g., "username" would mean this database
				is a username -> _VALUES_ mapping.
	_VALUES_		The data being stored for every key in
				the database. The order here is important,
				as it's the same order that you'll get
				when calling nast_get_result().
	_DELIM_			Only used internally to the NASTD server.

As noted above, the B<_VALUES_> key shows what data you can find in the
NASTD database, as well as the order it is returned in. This is why you
should investigate this key before trying to do any real work with
NASTD. You have to know what columns mean what values for any real
decision making to be done.

=head2 Server options

In order to fine-tune server performance and behaviour, it is possible
to set various server-side options through the C<nast_options_get()> and
C<nast_options_set()> APIs.

The interface is a bit clumsy at the low level - you pass in an array
to C<nast_options_set()> which has the options in a specific order, and
you get an array back from C<nast_options_get()> which contains the
options in a specific order.

It is recommended that before you call C<nast_options_set()> that you
first obtain the default options from the server through
C<nast_options_get()> and manipulate the values you care about. Then
pass that array back to C<nast_options_set()>.

The options you can set are as follows (remember to keep this order!):

	Index	C Option Name		Meaning (type)
	-----	-------------		--------------
	0	use_qcache		Whether or not to use the
					in-memory query cache. (BOOL)
	1	use_localdb		Whether or not to use the on-disk
					database. (BOOL)
	2	fallthrough_async	Whether or not to use an
					asynchronous API for fallthrough
					queries. (BOOL)
	3	always_fallthrough	Whether or not to always check
					the fallthrough cache over the
					local ones. Setting this to 1
					is the same as setting use_qcache
					and use_localdb to 0. (BOOL)
	4	fail_once		Whether or not to check a query
					in the local and in-memory storage
					once, and fail if the result isn't
					found the first time, then defer
					the next query to the fallthrough
					queue. (BOOL)
	5	no_fallthrough		Disable fallthrough queriers to
					MySQL server. (BOOL)

=head1 STATISTICS

The C<nast_stats()> function is used to gather server statistics. First
you call C<nast_stats()>, which grabs the statistics and stores them as
a result. Then you call C<nast_getresult()> as you would for a query.

The statistics come back in a human readable array, suitable for printing.

=head1 CONSTANTS

The only constants returned are via C<nast_geterr()>:

	Error Code		Meaning
	----------		-------
	NAST_OK			No errors occured.
	NAST_SERVER_GONE		The connection to the server no longer
				exists.
	NAST_NOMEM		The client has run out of memory performing
				an operation.
	NAST_UNKNOWN_RESPONSE	The server sent us a response we can't
				understand.
	NAST_TIMEDOUT		The soft timeout on a query has elapsed.
				This normally means the server is a bit
				bogged down, and the query should be
				retried.
	NAST_UNKNOWN_OPT		The server sent us an unknown option or
				we tried to set an unknown option.
	NAST_SERVER_ERR		Generic server side error. Check
				nast_errmsg() for more details.

=head1 SAMPLE PROGRAM

	#!/usr/bin/env perl

	use NASTD;

	$nasthole = &NASTD::nast_sphincter_new();
	if (!defined($nasthole)) {
		# Error message already printed.
		exit(1);
	}

	# Don't fallthrough to MySQL for this query.
	# First we get the default options from the server, then tweak
	# the ones we care about, and update the server with our options.
	@options = &NASTD::nast_options_get($nasthole);
	if (!defined(@options)) {
		print STDERR "Couldn't get options: " .
			&NASTD::nast_errmsg($nasthole) . "\n";
	}
	$options[5] = 1;

	if (&NASTD::nast_options_set($nasthole, @options) == -1) {
		print STDERR "Couldn't set options: " .
			&NASTD::nast_errmsg($nasthole) . "\n";
	}

	# Get some values.
	if (&NASTD::nast_get($nasthole, "shmit") == -1) {
		print STDERR "Couldn't perform get: " .
			&NASTD::nast_errmsg($nasthole) . "\n";
		&NASTD::nast_close_sphincter($nasthole);
		exit(1);
	}

	@vals = &NASTD::nast_get_result($nasthole);
	$nitems = $#vals + 1;
	print "Number of columns: $nitems.\n";
	for ($i = 0; $i < $nitems; $i++) {
		$val = shift(@vals);
		print "Result[$i]: $val\n";
	}

	&NASTD::nast_sphincter_close($nasthole);

=head1 BUGS

fail_once behaviour is not working as of this writing.

=head1 AUTHOR

Brian Cully <L<shmit@rcn.com|mailto:shmit@rcn.com>>

=cut
