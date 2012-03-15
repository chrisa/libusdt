#!/usr/bin/perl
use strict;
use warnings;
use IPC::Open3;
use Symbol 'gensym'; 
use IO::Handle;
use Test::More qw/ no_plan /;

my $arch;
if (scalar @ARGV == 1) {
    $arch = $ARGV[0];
}

run_tests('c', 'a');
run_tests('i', 1);

sub run_tests {
    my ($type, $start_arg) = @_;
    
    for my $i (0..6) {
        my ($status, $output) = run_dtrace('func', 'name', split(//, $type x $i));
        is($status, 0, 'dtrace exit status is 0');
        like($output, qr/func:name/, 'function and name match');
        
        my $arg = $start_arg;
        for my $j (0..$i - 1) {
            like($output, qr/arg$j:'$arg'/, "type '$type' arg $j is $arg");
            $arg++;
        }
    }
}

# --------------------------------------------------------------------------

sub gen_d {
    my (@types) = @_;

    my $d = 'testlibusdt*:::{ ';
    my $i = 0;
    for my $type (@types) {
        if ($type eq 'i') {
            $d .= "printf(\"arg$i:'%i' \", arg$i); ";
        }
        if ($type eq 'c') {
            $d .= "printf(\"arg$i:'%s' \", copyinstr(arg$i)); ";
        }
        $i++;
    }
    $d .= '}';

    return $d;
}
         
sub run_dtrace {
    my ($func, $name, @types) = @_;
    my $d = gen_d(@types);

    my $test;
    if (defined $arch) {
        $test = "./test_usdt$arch $func $name " . (join ' ', @types);
    }
    else {
        $test = "./test_usdt $func $name " . (join ' ', @types);
    }

    my @cmd = ('dtrace', '-Zn', $d, '-c', $test);

    my ($wtr, $rdr, $err);
    $err = gensym;
    my $pid = open3($wtr, $rdr, $err, @cmd);

    waitpid( $pid, 0 );
    my $status = $? >> 8;

    if ($status > 0) {
        while (!$err->eof) {
            print STDERR $err->getline;
        }
    }

    my ($header, $output) = ($rdr->getline, $rdr->getline);
    return ($status, $output || '');
}
