# SLOW!!!

package stkutils::data_packet;
use strict;

sub new {
	my $class = shift;
	my $data = shift;
	my $self = {};
	$self->{data} = ($data or '');
	$self->{init_length} = CORE::length($self->{data});
	bless($self, $class);
	return $self;
}
sub unpack {
	my $self = shift;
	my $template = shift;
die if !(defined $self);
die if !(defined $self->{data});
die if !(defined $template);
die if CORE::length($self->{data}) == 0;
	my @values = CORE::unpack($template.'a*', $self->{data});
die if $#values == -1;
	$self->{data} = splice(@values, -1);
die if !(defined $self->{data});
#print "@values\n";
	return @values;
}
sub pack {
	my $self = shift;
	my $template = shift;
die if !(defined($template));
die if !(defined(@_));
die unless defined $_[0];
#print "@_\n";
	$self->{data} .= CORE::pack($template, @_);
}
use constant template_for_scalar => {
	h32	=> 'V',
	h16	=> 'v',
	h8	=> 'C',
	u32	=> 'V',
	u16	=> 'v',
	u8	=> 'C',
	s32	=> 'l',
	s16	=> 'v',
	s8	=> 'C',
	sz	=> 'Z*',
	f32	=> 'f',
	guid	=> 'a[16]',
};
use constant template_for_vector => {
	l8u8v	=> 'C/C',
	l32u8v	=> 'V/C',
	l32u16v	=> 'V/v',
	l32u32v	=> 'V/V',
	l32szv	=> 'V/(Z*)',
	l8szbv	=> 'C/(Z*C)',
	u8v8	=> 'C8',
	u8v4	=> 'C4',
	f32v3	=> 'f3',
	f32v4	=> 'f4',
	s32v3	=> 'l3',
	s32v4	=> 'l4',
	h32v3	=> 'V3',
	h32v4	=> 'V4',	
	actorData => 'C171',
	ha1	=> 'C12',
	ha2	=> 'C8',
};
sub unpack_properties {
	my $self = shift;
	my $container = shift;
	foreach my $p (@_) {
#print "$p->{name} = ";
		if ($p->{type} eq 'shape') {
			my ($count) = $self->unpack('C');
			while ($count--) {
				my %shape;
				($shape{type}) = $self->unpack('C');
				if ($shape{type} == 0) {
					@{$shape{sphere}} = $self->unpack('f4');
				} elsif ($shape{type} == 1) {
					@{$shape{box}} = $self->unpack('f12');
				} else {
					die;
				}
				push @{$container->{$p->{name}}}, \%shape;
			}
#		} elsif ($p->{type} eq 'ordaf') {
#			my ($count) = $self->unpack('V');
#			if ($count == 0) {
#				($container->{$p->{name}}) = 0;
#			}
#			while ($count--) {
#				my %obj;
#				($obj{name}, $obj{number}) = $self->unpack('Z*V');
#				my ($inner_count) = $self->unpack('V');
#				while ($inner_count--) {
#					my %afs; 
#					($afs{af_section}) = $self->unpack('Z*VV');
#					push @{$obj{af_sects}}, \%afs;
#				}
#				push @{$container->{$p->{name}}}, \%obj;
#			}
		} elsif ($p->{type} eq 'suppl') {
			my ($count) = $self->unpack('V');
			while ($count--) {
				my %obj;
				($obj{section_name}, $obj{item_count}, $obj{min_factor}, $obj{max_factor}) = $self->unpack('Z*Vff');
				push @{$container->{$p->{name}}}, \%obj;
			}
		} elsif ($p->{type} eq 'f32v4') {				#let's shut up #QNAN warnings.
			my @buf = $self->unpack('f4');
			my $i = 0;
			while ($i < 4) {
				if (!defined ($buf[$i] <=> 9**9**9)) {
#					print "replacing bad float $buf[$i]...\n";
					$buf[$i] = 0;
				}
				$i++;
			}
			@{$container->{$p->{name}}} = @buf;
		} elsif ($p->{type} eq 'f32v3') {				
			my @buf = $self->unpack('f3');
			my $i = 0;
			while ($i < 3) {
				if (!defined ($buf[$i] <=> 9**9**9)) {
#					print "replacing bad float $buf[$i]...\n";
					$buf[$i] = 0;
				}
				$i++;
			}
			@{$container->{$p->{name}}} = @buf;
		} elsif ($p->{type} eq 'afspawns' or $p->{type} eq 'afspawns_u32') {
			my ($count) = $self->unpack('v');
			while ($count--) {
				my %obj;
				if ($p->{type} eq 'afspawns') {
					($obj{section_name}, $obj{weight}) = $self->unpack('Z*f');
				} else {
					($obj{section_name}, $obj{weight}) = $self->unpack('Z*V');
				}
				push @{$container->{$p->{name}}}, \%obj;
			}
		} else {
			my $template = template_for_scalar->{$p->{type}};
			if (defined $template) {
				($container->{$p->{name}}) = $self->unpack($template);
				if ($p->{type} eq 'sz') {
					chomp $container->{$p->{name}};
					$container->{$p->{name}} =~ s/\r//g;
				}
			} elsif ($p->{type} eq 'u24') {
				($container->{$p->{name}}) = CORE::unpack('V', CORE::pack('CCCC', $self->unpack('C3'), 0));
			} elsif ($p->{type} eq 'q16') {
				my ($qf) = $self->unpack('v');
				($container->{$p->{name}}) = convert_q16($qf);
			} elsif ($p->{type} eq 'q16_old') {
				my ($qf) = $self->unpack('v');
				($container->{$p->{name}}) = convert_q16_old($qf);
			} elsif ($p->{type} eq 'q8') {
				my ($q8) = $self->unpack('C');
				($container->{$p->{name}}) = convert_q8($q8);
			} elsif ($p->{type} eq 'q8v3') {
				my (@q8) = $self->unpack('C3');
				my $i = 0;
				while ($i < 3) {
					@{$container->{$p->{name}}}[$i] = convert_q8($q8[$i]);
					$i++;
				}
			} elsif ($p->{type} eq 'q8v4') {
				my (@q8) = $self->unpack('C4');
				my $i = 0;
				while ($i < 4) {
					@{$container->{$p->{name}}}[$i] = convert_q8($q8[$i]);
					$i++;
				}
			} else {
				@{$container->{$p->{name}}} = $self->unpack(template_for_vector->{$p->{type}});
			}
		}
	}
}
sub pack_properties {
	my $self = shift;
	my $container = shift;

	foreach my $p (@_) {
#print "$p->{name} = ";
		my $template = template_for_scalar->{$p->{type}};
		if (defined $template) {
			$self->pack($template, $container->{$p->{name}});
		} elsif ($p->{type} eq 'shape') {
			$self->pack('C', $#{$container->{$p->{name}}} + 1);
			foreach my $shape (@{$container->{$p->{name}}}) {
				$self->pack('C', $$shape{type});
				if ($$shape{type} == 0) {
					$self->pack('f4', @{$$shape{sphere}});
				} elsif ($$shape{type} == 1) {
					$self->pack('f12', @{$$shape{box}});
				}
			}
		} elsif ($p->{type} eq 'u24') {
			$self->pack('CCC', CORE::unpack('CCCC', CORE::pack('V', $container->{$p->{name}})));
		} elsif ($p->{type} eq 'q16') {
			my $f16 = $container->{$p->{name}};
			$self->pack("v", convert_u16($f16));
		} elsif ($p->{type} eq 'q16_old') {
			my $f16 = $container->{$p->{name}};
			$self->pack("v", convert_u16_old($f16));
		} elsif ($p->{type} eq 'suppl') {
			$self->pack('V', $#{$container->{$p->{name}}} + 1);
			foreach my $sect (@{$container->{$p->{name}}}) {
				$self->pack('Z*Vff', $$sect{section_name}, $$sect{item_count}, $$sect{min_factor}, $$sect{max_factor});
			}
		} elsif ($p->{type} eq 'afspawns') {
			$self->pack('v', $#{$container->{$p->{name}}} + 1);
			foreach my $sect (@{$container->{$p->{name}}}) {
				$self->pack('Z*f', $$sect{section_name}, $$sect{weight});
			}
		} elsif ($p->{type} eq 'afspawns_u32') {
			$self->pack('v', $#{$container->{$p->{name}}} + 1);
			foreach my $sect (@{$container->{$p->{name}}}) {
				$self->pack('Z*V', $$sect{section_name}, $$sect{weight});
			}
		} elsif ($p->{type} eq 'q8') {
			my $f8 = $container->{$p->{name}};
			$self->pack("C", convert_u8($f8));
#		} elsif ($p->{type} eq 'ordaf') {
#			$self->pack('V', $#{$container->{$p->{name}}} + 1);
#			my $i = 0;
#			foreach my $sect (@{$container->{$p->{name}}}) {
#				$self->pack('Z*V', $$sect[0..1]);
#				$self->pack('V', $#{$sect[2]} + 1);
#				my $k = 0;
#				foreach my $af_sect (@{$sect[2]}) {
#					$self->pack('Z*VV', $af_sect[0], $af_sect[1], $af_sect[2]);
#					$k++;
#				}
#				$i++;
#			}
		} else {
			my $n = $#{$container->{$p->{name}}} + 1;
			if ($p->{type} eq 'l32u16v') {
				$self->pack("Vv$n", $n, @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 'l32u8v') {
				$self->pack("VC$n", $n, @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 'l32u32v') {
				$self->pack("VV$n", $n, @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 'l32szv') {
				$self->pack("V(Z*)$n", $n, @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 'l8u8v') {
				$self->pack("CC$n", $n, @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 'u8v8' or $p->{type} eq 'u8v4') {
				$self->pack("C$n", @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 'f32v3') {
				$self->pack('f3', @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 'f32v4') {
				$self->pack('f4', @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 's32v3') {
				$self->pack('l3', @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 's32v4') {
				$self->pack('l4', @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 'q8v') {
				$self->pack("C$n", @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 'q8v3' or $p->{type} eq 'q8v4') {
				foreach my $u8 (@{$container->{$p->{name}}}) {
					$self->pack('C', convert_u8($u8));
				}
			} elsif ($p->{type} eq 'actorData') {
				$self->pack('C171', @{$container->{$p->{name}}});	
			} elsif ($p->{type} eq 'ha1') {
				$self->pack('C12', @{$container->{$p->{name}}});	
			} elsif ($p->{type} eq 'ha2') {
				$self->pack('C8', @{$container->{$p->{name}}});	
			} elsif ($p->{type} eq 'l8szbv') {
				$n = $n/2;
				$self->pack("C(Z*C)$n", $n, @{$container->{$p->{name}}});
			} elsif ($p->{type} eq 'h32v3') {
				$self->pack('V3', @{$container->{$p->{name}}});	
			} elsif ($p->{type} eq 'h32v4') {
				$self->pack('V4', @{$container->{$p->{name}}});					
			} elsif ($p->{type} eq 'actorData') {
				$self->pack('C171', @{$container->{$p->{name}}});						
			} else {
				die;
			}
		}
	}
}
sub length {
	return CORE::length($_[0]->{data});
}
sub r_tell {
	return $_[0]->{init_length} - CORE::length($_[0]->{data});
}
sub w_tell {
	return CORE::length($_[0]->{data});
}
sub data {
	return $_[0]->{data};
}
sub convert_q8 {
	my ($u) = @_;
	my $q = ($u / 255.0);
	return $q;
}
sub convert_u8 {
	my ($q) = @_;
	my $u = int ($q * 255.0);
	return $u;
}
sub convert_q16 {
	my ($u) = @_;
	my $q = (($u / 43.69) - 500.0);
	return $q;
}
sub convert_u16 {
	my ($q) = @_;
	my $u = (($q + 500.0) * 43.69);
	return $u;
}
sub convert_q16_old {
	my ($u) = @_;
	my $q = (($u / 32.77) - 1000.0);
	return $q;
}
sub convert_u16_old {
	my ($q) = @_;
	my $u = (($q + 1000.0) * 32.77);
	return $u;
}
1;
