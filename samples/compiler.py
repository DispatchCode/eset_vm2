import re
import sys
import struct
import itertools


class Opcode(object):
    
    def __init__(self, code, args):
        self.code = code
        self.args = args


class Assembler(object):

    class Error(RuntimeError):
        pass

    Opcodes = {

        "mov":             Opcode("000", "RR"),        # 000 mov r1, r2
        "loadConst":       Opcode("001", "CR"),        # 001 loadConst const, r1

        "add":             Opcode("010001", "RRR"),    # 010001 add r1, r2, r3
        "sub":             Opcode("010010", "RRR"),    # 010010 sub r1, r2, r3
        "div":             Opcode("010011", "RRR"),    # 010011 div r1, r2, r3
        "mod":             Opcode("010100", "RRR"),    # 010100 mod r1, r2, r3
        "mul":             Opcode("010101", "RRR"),    # 010101 mul r1, r2, r3

        "compare":         Opcode("01100", "RRR"),     # 01100 compare r1, r2, r3 -> (-1, 0, 1)
        "jump":            Opcode("01101", "L"),       # 01101 jump code-offset
        "jumpEqual":       Opcode("01110", "LRR"),     # 01110 jumpEqual code-offset, r1, r2

        "read":            Opcode("10000", "RRRR"),    # 10000 read r-offset, r-count, r-outputaddress, r-readsize
        "write":           Opcode("10001", "RRR"),     # 10001 write r-offset, r-count, r-outputaddress
        "consoleRead":     Opcode("10010", "R"),       # 10010 consoleRead r1
        "consoleWrite":    Opcode("10011", "R"),       # 10011 consoleWrite r1

        "createThread":    Opcode("10100", "LR"),      # 10100 createThread code-offset, index
        "joinThread":      Opcode("10101", "R"),       # 10101 joinThread index
        "hlt":             Opcode("10110", ""),        # 10110 hlt
        "sleep":           Opcode("10111", "R"),       # 10111 sleep time

        "call":            Opcode("1100", "L"),        # 1100 call code-offset
        "ret":             Opcode("1101", ""),         # 1101 ret

        "lock":            Opcode("1110", "R"),        # 1110 lock index
        "unlock":          Opcode("1111", "R"),        # 1111 unlock index
    }

    DataAccessTypes = {

        "byte": "00",
        "word": "01",
        "dword": "10",
        "qword": "11",
    }

    __bytecode = None
    __offsets_list = None
    __offset_patches = None

    def __init__(self, parser):
        self.__parser = parser

    def __assemble_register(self, argument_data):
        register_id, reference_type = argument_data

        if reference_type:  # 1 xx xxxx - memory access using register value as offset
            self.__bytecode += "1" + self.DataAccessTypes[reference_type][::-1] + "{0:04b}".format(register_id)[::-1]

        else:  # 0 xxxx - register (4 bit index for 16 total registers)
            self.__bytecode += "0" + "{0:04b}".format(register_id)[::-1]

    def __assemble_constant(self, argument_data):
        self.__bytecode += "{0:064b}".format(int(argument_data, 0))[::-1]

    def __assemble_label(self, argument_data):

        offset = self.__parser.code_labels.get(argument_data)
        if offset is None:
            raise Assembler.Error("Undefined code label %s" % argument_data)

        if offset < len(self.__offsets_list):
            address = self.__offsets_list[offset]

        else:  # let's patch it in the end
            address = 0
            self.__offset_patches[len(self.__bytecode)] = offset

        self.__bytecode += "{0:032b}".format(address)[::-1]

    def __apply_patches(self):

        for position, offset in self.__offset_patches.iteritems():
            self.__bytecode[position:position+32] = "{0:032b}".format(self.__offsets_list[offset])[::-1]

    def build(self, filepath):

        self.__bytecode = bytearray()
        self.__offsets_list = []
        self.__offset_patches = {}

        for instruction in self.__parser.code_section:

            self.__offsets_list.append(len(self.__bytecode))

            opcode = Assembler.Opcodes[instruction[0]]
            self.__bytecode += opcode.code

            for argument_type, argument_data in zip(opcode.args, instruction[1:]):

                if argument_type == "R":
                    self.__assemble_register(argument_data)

                elif argument_type == "C":
                    self.__assemble_constant(argument_data)

                elif argument_type == "L":
                    self.__assemble_label(argument_data)

        self.__apply_patches()

        actual_data_size = len(self.__parser.data_section)

        if actual_data_size > self.__parser.data_size:
            print "Warning: bad .dataSize, was %d but used %d, expanding" % (
                self.__parser.data_size, actual_data_size)

            self.__parser.data_size = actual_data_size

        if len(self.__bytecode) % 8 != 0:
            self.__bytecode += "0" * (8 - len(self.__bytecode) % 8)

        with open(filepath, "wb") as handle:
            handle.write("ESET-VM2")

            handle.write(struct.pack("I", len(self.__bytecode) / 8))
            handle.write(struct.pack("I", self.__parser.data_size))
            handle.write(struct.pack("I", len(self.__parser.data_section)))

            for x in xrange(0, len(self.__bytecode), 8):
                handle.write(struct.pack("B", int(str(self.__bytecode[x:x+8]), 2)))

            for data_byte in self.__parser.data_section:
                handle.write(struct.pack("B", data_byte))


class Lexer(object):

    def __init__(self, filepath):
        self.__filepath = filepath

    def analyse(self):
        with open(self.__filepath, "r") as handle:

            for line_no, line in zip(itertools.count(), handle):
                tokens = line.split("#", 2)[0].split()

                if len(tokens) == 0:
                    continue

                yield line_no + 1, line, tokens


class Parser(object):
    
    class Error(RuntimeError):
        pass
    
    class ParseMode(object):
        Data = 1
        Code = 2

    __parse_mode = None

    data_size = None
    data_labels = None
    data_section = None

    code_labels = None
    code_section = None

    last_parsed_line = ""
    last_parsed_line_no = -1

    def __init__(self, lexer):
        self.__lexer = lexer

    def __parse_section(self, tokens):

        if tokens[0] == '.dataSize':
            if self.data_size is not None:
                raise Parser.Error("Double data size spotted")

            self.data_size = int(tokens[1])

        elif tokens[0] == '.code':
            self.__parse_mode = self.ParseMode.Code

        elif tokens[0] == '.data':
            self.__parse_mode = self.ParseMode.Data

        else:
            raise Parser.Error("Bad token")

    def __parse_label(self, tokens):
        label = tokens[0][:-1]

        if self.__parse_mode == self.ParseMode.Code:
            if label in self.code_labels:
                raise Parser.Error("Duplicated label")

            self.code_labels[label] = len(self.code_section)

        elif self.__parse_mode == self.ParseMode.Data:
            if label in self.data_labels:
                raise Parser.Error("Duplicated label")

            self.data_labels[label] = len(self.data_section)

        else:
            raise Parser.Error("Bad label")
        
    @staticmethod
    def __translate_argument_tokens(tokens):

        if len(tokens) == 0:
            return []

        return re.split(r"\s*,\s*", " ".join(tokens))

    __register_val_pattern = r"r(?P<id>[0-9]+)"

    __register_ref_pattern = r"(?P<ref>%s)\s*\[\s*%s\s*\]" % (
        "|".join(map(re.escape, Assembler.DataAccessTypes.iterkeys())), __register_val_pattern)

    def __parse_code(self, tokens):
        opcode_name = tokens[0]

        opcode = Assembler.Opcodes.get(opcode_name)
        if not opcode:
            raise Parser.Error("Bad opcode [%s]" % opcode_name)

        argument_data = self.__translate_argument_tokens(tokens[1:])

        if len(argument_data) != len(opcode.args):
            raise Parser.Error("Bad opcode argument count")

        instruction = [opcode_name]

        for argument_type, argument_data in zip(opcode.args, argument_data):

            if argument_type == "R":  # register
                match = None
                for pattern in (self.__register_val_pattern, self.__register_ref_pattern):

                    match = re.match(pattern, argument_data)
                    if match:
                        break

                if not match:
                    raise Parser.Error("Bad register argument type [%s] (syntax error)" % argument_data)

                groups = match.groupdict()

                register_id = int(groups["id"])
                if register_id > 16:
                    raise Parser.Error("Bad register argument type (too big)")

                instruction.append([register_id, groups.get("ref")])

            elif argument_type == "C":  # constant
                instruction.append(argument_data)

            elif argument_type == "L":  # label
                instruction.append(argument_data)

            else:
                raise NotImplementedError("Bad argument type")

        self.code_section.append(instruction)

    def __parse_data(self, tokens):

        for entry in tokens:

            value = int(entry, 16)
            if value > 255:
                raise Parser.Error("Bad value in line")

            self.data_section.append(value)

    def analyse(self):

        self.__parse_mode = None

        self.data_size = None
        self.data_labels = {}
        self.data_section = []

        self.code_labels = {}
        self.code_section = []

        for self.last_parsed_line_no, self.last_parsed_line, tokens in self.__lexer.analyse():

            if tokens[0][0] == '.':
                self.__parse_section(tokens)

            elif len(tokens) == 1 and tokens[0][-1] == ':':
                self.__parse_label(tokens)

            elif self.__parse_mode == self.ParseMode.Code:
                self.__parse_code(tokens)

            elif self.__parse_mode == self.ParseMode.Data:
                self.__parse_data(tokens)

            else:
                raise Parser.Error("Bad token")


if __name__ == "__main__":

    def main(input_filepath, output_filepath):

        parser = Parser(Lexer(input_filepath))
        try:
            parser.analyse()

        except Parser.Error as exc:
            print "Parser error on line %d: %s" % (parser.last_parsed_line_no, exc.message)
            sys.exit(2)

        assembler = Assembler(parser)
        try:
            assembler.build(output_filepath)

        except Assembler.Error as exc:
            print "Assembler error: %s" % exc.message
            sys.exit(3)

        print "All ok"

    if len(sys.argv) != 3:
        print 'Usage: %s <input> <output>' % sys.argv[0]
        sys.exit(1)

    main(sys.argv[1], sys.argv[2])
