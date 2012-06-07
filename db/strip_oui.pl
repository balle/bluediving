#!/usr/bin/perl

open(DB, "<$ARGV[0]") or die "$!\n";
open(OUT, ">$ARGV[0].new") or die "$!\n";

while(<DB>)
{
   if($_ =~ /\(hex\)/)
   {
      print OUT $_;
   }
}

close(OUT);
close(DB);
