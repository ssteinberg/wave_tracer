import gdb.printing
import math
from termcolor import colored


quantity_colour = 'cyan'
variable_colour = 'yellow'


def print_quantity_type(type):
    s = str(gdb.types.get_basic_type(type))

    if (not s.startswith('mp_units::quantity')):
        s = s[s.find('mp_units::quantity<'):].removesuffix(' >')

    if (s.find('mp_units::quantity<mp_units::reference<mp_units::isq::thermodynamic_temperature, mp_units::si::kelvin>(),') >= 0):
        s = 'K'
    elif (s.find('mp_units::quantity<mp_units::reference<mp_units::isq::thermodynamic_temperature, mp_units::si::deg_C>(),') >= 0):
        s = '°C'
    elif (s.find('mp_units::quantity<mp_units::reference<mp_units::angular::angle, mp_units::angular::radian>(), ') >= 0):
        s = 'rad'
    elif (s.find('mp_units::quantity<mp_units::reference<mp_units::angular::angle, mp_units::angular::degree>(), ') >= 0):
        s = '°'
    elif (s.find('mp_units::quantity<mp_units::reference<mp_units::isq::length, mp_units::si::metre>(), ') >= 0):
        s = 'm'
    elif (s.find('mp_units::quantity<mp_units::reference<mp_units::derived_quantity_spec<mp_units::dimensionless, mp_units::per<mp_units::isq::length> >, mp_units::derived_unit<mp_units::one, mp_units::per<mp_units::si::milli_<mp_units::si::metre> > > >(), ') >= 0):
        s = '1/mm'
    elif (s.find('mp_units::quantity<mp_units::reference<mp_units::isq::length, mp_units::si::milli_<mp_units::si::metre> >(), ') >= 0):
        s = 'mm'
    else:
        s = s.removeprefix('mp_units::quantity<').removesuffix('>')\
            .replace('mp_units::','')\
            .replace('derived_unit','')\
            .replace('derived_quantity_spec','spec')\
            .replace('power<','pow<')\
            .replace(' >','>')\
            .replace('dimensionless','1')\
            .replace('one','1')\
            .replace('thermodynamic_temperature','temp')\
            .replace('()','')\
            .replace('isq::','')\
            .replace('si::','')\
            .replace(', per<','/<')\
            .removeprefix('detail::reference_t')\
            .removeprefix('reference')\
            .rstrip().rsplit(' ', 1)[0].rstrip().removesuffix(',')\
            .removeprefix('<').removesuffix('>')
    return colored('['+s+']', quantity_colour)


class vec2Printer:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return '{' + colored(str(self.val['x']), attrs=["bold"]) +','+ colored(str(self.val['y']), attrs=["bold"]) + '}'

class vec3Printer:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return '{' + colored(str(self.val['x']), attrs=["bold"]) +','+ colored(str(self.val['y']), attrs=["bold"]) +','+ colored(str(self.val['z']), attrs=["bold"]) + '}'

class vec4Printer:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return '{' + colored(str(self.val['x']), attrs=["bold"]) +','+ colored(str(self.val['y']), attrs=["bold"]) +','+ colored(str(self.val['z']), attrs=["bold"]) +','+ colored(str(self.val['w']), attrs=["bold"]) + '}'

class qvec2Printer:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return '{' + \
            colored(str(self.val['x']['numerical_value_is_an_implementation_detail_']), attrs=["bold"]) +','+ \
            colored(str(self.val['y']['numerical_value_is_an_implementation_detail_']), attrs=["bold"]) + '} ' + print_quantity_type(self.val.type)

class qvec3Printer:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return '{' + \
            colored(str(self.val['x']['numerical_value_is_an_implementation_detail_']), attrs=["bold"]) +','+ \
            colored(str(self.val['y']['numerical_value_is_an_implementation_detail_']), attrs=["bold"]) +','+ \
            colored(str(self.val['z']['numerical_value_is_an_implementation_detail_']), attrs=["bold"]) + '} ' + print_quantity_type(self.val.type)

class qvec4Printer:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return '{' + \
            colored(str(self.val['x']['numerical_value_is_an_implementation_detail_']), attrs=["bold"]) +','+ \
            colored(str(self.val['y']['numerical_value_is_an_implementation_detail_']), attrs=["bold"]) +','+ \
            colored(str(self.val['z']['numerical_value_is_an_implementation_detail_']), attrs=["bold"]) +','+ \
            colored(str(self.val['w']['numerical_value_is_an_implementation_detail_']), attrs=["bold"]) + '} ' + print_quantity_type(self.val.type)

class rayPrinter:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return str(self.val['o']) + ' -> ' + str(self.val['d'])

class ellipticConePrinter:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        e = self.val['e']
        x = self.val['x']
        ecc = math.sqrt(max(0,1-1/(e*e)))
        alpha = math.atan(self.val['tan_alpha'])

        if e>=1:
            ecc_text = colored('ε',variable_colour) + '=' + colored(str(ecc), attrs=["bold"])
        else:
            ecc_text = colored('e',variable_colour) + '=' + colored(str(e), 'red', attrs=["bold"])

        return '( ' + str(self.val['r']) + ', ' +\
                    colored('α', variable_colour) + '=' + colored(str(alpha), attrs=["bold"]) + ', '+\
                    ecc_text +  ', '+\
                    colored('x', variable_colour) + '=' + colored(str(self.val['tangent']), attrs=["bold"]) + ' ⨯ ' + colored(str(self.val['initial_x_length']), attrs=["bold"]) +\
                ' )'

class beamPrinter:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return '( ' +\
                    colored('S', variable_colour) + '=' + str(self.val['S']) + ', '+\
                    colored('envelope', variable_colour) + '=' + str(self.val['envelope']) + ', '+\
                    colored('self_intersection_distance', variable_colour) + '=' + colored(str(self.val['self_intersection_distance']), attrs=["bold"]) + ', '+\
                    colored('intersection_offset', variable_colour) + '=' + colored(str(self.val['intersection_offset']), attrs=["bold"]) + ', '+\
                ' )'

class quantityPrinter:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return colored(str(self.val['numerical_value_is_an_implementation_detail_']), attrs=["bold"]) + " " + print_quantity_type(self.val.type)

class quantityPointPrinter:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return colored(str(self.val['quantity_from_origin_is_an_implementation_detail_']), attrs=["bold"]) 

class emptyUnitPrinter:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return 'unitless'

class namedUnitPrinter:
    def __init__(self, val):
        self.val = val

    def to_string (self):
        return colored(str(self.val['_symbol_']['utf8_']['data_']), quantity_colour)


def build_prettyprinters():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("wt")

    pp.add_printer('vec2_t', '^glm::vec<2,.*>$', vec2Printer)
    pp.add_printer('vec3_t', '^glm::vec<3,.*>$', vec3Printer)
    pp.add_printer('vec4_t', '^glm::vec<4,.*>$', vec4Printer)
    pp.add_printer('dir2_t', '^wt::unit_vector<2,.*>$', vec2Printer)
    pp.add_printer('dir3_t', '^wt::unit_vector<3,.*>$', vec3Printer)
    pp.add_printer('dir4_t', '^wt::unit_vector<4,.*>$', vec4Printer)

    pp.add_printer('qvec2_t', '^wt::quantity_vector<2,.*>$', qvec2Printer)
    pp.add_printer('qvec3_t', '^wt::quantity_vector<3,.*>$', qvec3Printer)
    pp.add_printer('qvec4_t', '^wt::quantity_vector<4,.*>$', qvec4Printer)

    pp.add_printer('ray_t',  '^wt::ray_t$', rayPrinter)
    pp.add_printer('elliptic_cone_t', '^wt::elliptic_cone_t$', ellipticConePrinter)

    pp.add_printer('quantity', '^mp_units::quantity<.*>$', quantityPrinter)
    pp.add_printer('quantity_point', '^mp_units::quantity_point<.*>$', quantityPointPrinter)
    pp.add_printer('mp_units::unit', '^mp_units::detail::derived_unit_impl<>$', emptyUnitPrinter)
    pp.add_printer('mp_units::named_unit', '^mp_units::named_unit<.*>$', namedUnitPrinter)

    pp.add_printer('beam_t', '^wt::beam_t$', beamPrinter)

    return pp

gdb.printing.register_pretty_printer(gdb.current_objfile(),build_prettyprinters())

