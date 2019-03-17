use strict;
use warnings;

use DBI;

my $dbh = DBI->connect("DBI:SQLite:dbname=dict.sqlite", "", "");

$dbh->do("CREATE TABLE dict (jap TEXT, eng TEXT)");
$dbh->do('BEGIN TRANSACTION');

my $sth = $dbh->prepare("INSERT INTO dict VALUES (?, ?)");

{
	open( my $in, "edict.u8" );
	while( my $line = <$in> )
	{
		chomp $line;
		my $idx_space = index $line, ' ';
		next unless $idx_space > 0;
		my $jap = substr $line, 0, $idx_space;
		my $eng = substr $line, $idx_space+1;

		#print $jap, "\t", $eng, "\n";
		$sth->bind_param(1, $jap);
		$sth->bind_param(2, $eng);
		$sth->execute;
	}
	close $in;
}

$dbh->do('COMMIT');
$dbh->disconnect;
