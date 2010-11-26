#!/usr/bin/perl -w
#

# language-IDs file downloadable from usb.org (http://www.usb.org/developers/docs/USB_LANGIDs.pdf)
$USBfile1 = "langIDs.txt";

$IDlist{0} = 0;
$VarName = "languageIDmap";

sub trim($)
{
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	#special
	$string =~ s/\"/'/;
	return $string;
}


# read input from official usb vendor file and store everything in an associative array
# (needed because we want to sort the whole thing)
open (USBFILE, $USBfile1) or die("Cannot read usb file: $!");
while (<USBFILE>) {
	($langID,$langName)=split(/ /, $_, 2);
	$langName = trim($langName);
#	print "$langID -> $langName\n";
	next if /^#/;
	my $iLID = hex($langID);
	if ( $iLID <= 0 ) {next;}

	$IDlist{$iLID} = $langName;
}
close( USBFILE );

# iterate over hash and sort key numeric
foreach $id (sort { $a <=> $b } (keys %IDlist)) {
	if ( $id <= 0 ) { next; }
	print "${VarName}[$id] = \"$IDlist{$id}\";\n"
}
