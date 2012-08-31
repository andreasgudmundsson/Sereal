use strict;
use warnings;
use blib;
use Sereal::Encoder qw(encode_sereal);
use Benchmark qw(cmpthese :hireswallclock);
use JSON::XS qw(encode_json);
use Data::Dumper qw(Dumper);
use Data::Dumper::Limited qw(DumpLimited);
use Storable qw(nfreeze thaw);
use Data::MessagePack;
use Getopt::Long qw(GetOptions);

GetOptions(
  'dump|d' => \(my $dump),
);

our %opt = @ARGV;

use constant SEREAL_ONLY => 0;

our $mpo = Data::MessagePack->new();

srand(0);
my @str;
push @str, join("", map chr(65+int(rand(57))), 1..10) for 1..1000;
my @rand = map rand,1..1000;

# Independent, virgin data set for each serializer
our %data;
$data{$_}= [
  [1..10000],
  {@str},
  {@str},
  [1..10000],
  {@str},
  [@rand],
  {@str},
  {@str},
] for qw(sereal sereal_func dd1 dd2 ddl mp json_xs storable);

our $enc = Sereal::Encoder->new(\%opt);
my ($json_xs, $dd1, $dd2, $ddl, $sereal, $storable, $mp);
$sereal   = $enc->encode($data{sereal});
if (!SEREAL_ONLY) {
  $json_xs  = encode_json($data{json_xs});
  $dd1      = Data::Dumper->new([$data{dd1}])->Indent(0)->Dump();
  $dd2      = Dumper($data{dd2});
  $ddl      = DumpLimited($data{ddl});
  $mp       = $mpo->pack($data{mp});
  $storable = nfreeze($data{storable}); # must be last
}

print($sereal), exit if $dump;

my $sereal_len= bytes::length($sereal);
require bytes;
if (!SEREAL_ONLY) {
    for my $tuple (
        ["JSON::XS",  bytes::length($json_xs)],
        ["Data::Dumper::Limited", bytes::length($ddl)],
        ["Data::Dumper (1)", bytes::length($dd1)],
        ["Data::Dumper (2)", bytes::length($dd2)],
        ["Storable" , bytes::length($storable)],
        ["Data::MessagePack" ,bytes::length($mp)],
        ["Sereal::Encoder",  bytes::length($sereal)],
    ) {
        my ($name, $size) = @$tuple;
        printf "%-40s %12d bytes %.2f%% of sereal\n", $name, $size, $size/$sereal_len *100;
    }
}

our $x;
cmpthese(
  -3,
  {
    (!SEREAL_ONLY
      ? (
        json_xs => '$::x = encode_json($::data{json_xs});',
        ddl => '$::x = DumpLimited($::data{ddl});',
        dd1 => '$::x = Data::Dumper->new([$::data{dd1}])->Indent(0)->Dump();',
        dd2 => '$::x = Dumper($::data{dd2});',
        storable => '$::x = nfreeze($::data{storable});',
        mp => '$::x = $::mpo->pack($::data{mp});',
      ) : ()),
    sereal_func => '$::x = encode_sereal($::data{sereal_func}, \%::opt);',
    sereal => '$::x = $::enc->encode($::data{sereal});',
  }
);

