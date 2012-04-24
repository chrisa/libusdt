#!/usr/bin/perl
use strict;
use warnings;
use IPC::Open3;
use Symbol 'gensym'; 
use IO::Handle;
use Test::More qw/ no_plan /;

my $USDT_ARG_MAX = 32;

my $arch;
if (scalar @ARGV == 1) {
    $arch = $ARGV[0];
}

my $user_t = ($^O eq 'solaris') ? 'uintptr_t' : 'user_addr_t';

run_tests('c', 'A');
run_tests('i', 1);

sub run_tests {
    my ($type, $start_arg) = @_;
    
    for my $i (0..$USDT_ARG_MAX) {
        my ($status, $output) = run_dtrace('func', 'name', split(//, $type x $i));
        is($status, 0, 'dtrace exit status is 0');
        like($output, qr/func:name/, 'function and name match');

        my $arg = $start_arg;
        for my $j (0..$i - 1) {
            like($output, qr/arg$j:'\Q$arg\E'/, "type '$type' arg $j is $arg");
            if ($type eq 'i') {
                $arg++;
            }
            else {
                $arg = chr(ord($arg) + 1);
            }
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
            $d .= "printf(\"arg$i:'%i' \", args[$i]); ";
        }
        if ($type eq 'c') {
            $d .= "printf(\"arg$i:'%s' \", copyinstr(($user_t)args[$i])); ";
        }
        $i++;
    }
    $d .= '}';

    return $d;
}
         
sub run_dtrace {
    my ($func, $name, @types) = @_;
    my $d = gen_d(@types);

    my @t_cmd;
    if (defined $arch) {
        @t_cmd = ("./test_usdt$arch", $func, $name, @types);
    }
    else {
        @t_cmd = ("./test_usdt", $func, $name, @types);
    }

    my ($d_wtr, $d_rdr, $d_err);
    my ($t_wtr, $t_rdr, $t_err);

    $d_err = gensym;
    $t_err = gensym;

    my $t_pid = open3($t_wtr, $t_rdr, $t_err, @t_cmd);
    my $enabled = $t_rdr->getline;

    my @d_cmd = ('dtrace', '-p', $t_pid, '-n', $d);

    my $d_pid = open3($d_wtr, $d_rdr, $d_err, @d_cmd);
    my $matched = $d_err->getline; # expect "matched 1 probe"

    $t_wtr->print("go\n");
    $t_wtr->flush;
    waitpid( $t_pid, 0 );

    my ($header, $output) = ($d_rdr->getline, $d_rdr->getline);
    waitpid( $d_pid, 0 );

    my $status = $? >> 8;
    if ($status > 0) {
        print STDERR $matched;
        while (!$d_err->eof) {
            print STDERR $d_err->getline;
        }
    }

    return ($status, $output || '');
}
