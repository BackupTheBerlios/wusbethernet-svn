#!/usr/bin/perl -w
#

# usb.if file downloadable from usb.org (http://www.usb.org/developers/tools/comp_dump)
$USBfile1 = "usb.if";
# usb.ids file  (http://www.linux-usb.org/usb.ids)
$USBfile2 = "usb.ids";

$IDlist{0} = 0;
$VarName = "vendorIDmap";

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
	($vendorID,$VendorName)=split(/\|/, $_);
	$VendorName = trim($VendorName);
	next if /^#/;
	my $iVID = int($vendorID);
	if ( $iVID <= 0 ) {next;}

	$IDlist{$iVID} = $VendorName;
}
close( USBFILE );

# read input from official usb vendor file and store everything in an associative array
# (needed because we want to sort the whole thing)
open (USBFILE, $USBfile2) or die("Cannot read usb file: $!");
while (<USBFILE>) {
	# skip: comments, empty lines, indented lines and lines beginning with an upper-case letter (not vendor ids)
	next if /^[ ]*#/;
	next if /^[ \t]*$/;
	next if /^[A-Z]/;
	next if /^ /;
	next if /^\t/;

	($vendorID,$VendorName)=split(/ +/, $_,2);
	$VendorName = trim($VendorName);
	$vendorID = trim($vendorID);
	my $iVID = hex("0x" . $vendorID );
	if ( $iVID <= 0 ) {next;}

	if ( !defined($IDlist{$iVID}) ) {
		$IDlist{$iVID} = $VendorName;
#		print "$iVID -> \"$VendorName\"\n";
	}
}
close(USBFILE);

# iterate over hash and sort key numeric
foreach $id (sort { $a <=> $b } (keys %IDlist)) {
	if ( $id <= 0 ) { next; }
	print "${VarName}[$id] = \"$IDlist{$id}\";\n"
}
