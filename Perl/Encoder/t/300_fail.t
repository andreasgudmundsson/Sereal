#!perl
use strict;
use warnings;
use Sereal::Encoder;
use Sereal::Encoder::Constants qw(:all);
use File::Spec;

use lib File::Spec->catdir(qw(t lib));
BEGIN {
  lib->import('lib')
    if !-d 't';
}

use Sereal::TestSet qw(:all);

use Test::More tests => 8;
my ($ok, $err, $out);

# croak_on_bless test
SCOPE: {
    my $e = Sereal::Encoder->new({
        croak_on_bless => 1,
    });
    $ok = eval{$out = $e->encode(bless({}, "Foo")); 1};
    $err = $@ || 'Zombie error';

    ok(!$ok, "Object throws exception");
    ok($err =~ /object/i, 'Exception refers to object');

    $ok =  eval {$out = $e->encode({}); 1};
    ok($ok, "Non-blessed hash does not throw exception");

    # test that code refs throw exception
    $ok = eval {$out = $e->encode(sub {}); 1};
    ok(!$ok, "Code ref throws exception");
}

# test that code refs with undef_unknown don't throw exceptions
SCOPE: {
    my $e = Sereal::Encoder->new({undef_unknown => 1});
    $ok = eval {$out = $e->encode(sub{}); 1};
    $err = $@ || 'Zombie error';
    ok($ok, "undef_unknown makes CODE encoding not fail");
    local $TODO = "We still output \\undef instead of undef";
    is($out, $Header . chr(0) . chr(SRL_HDR_UNDEF),
       "output is undef")
    or do {
        hobodecode($out) if $ENV{DEBUG_SEREAL};
    }
}

# test that code refs with stringify_unknown don't throw exceptions
SCOPE: {
    my $e = Sereal::Encoder->new({undef_unknown => 1});
    my $sub = sub{};
    $ok = eval {$out = $e->encode($sub); 1};
    $err = $@ || 'Zombie error';
    ok($ok, "undef_unknown makes CODE encoding not fail");
    local $TODO = "We still output \\undef instead of undef";
    my $str = $e->encode("$sub");
    is($out, $str, "output is stringified ref")
    or do {
        hobodecode($out) if $ENV{DEBUG_SEREAL};
    }
}
