#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef char byte;
typedef short int word;
typedef unsigned short int adr;

byte mem[(64)*1024];
word reg[8];

#define pc reg[7]
#define sp reg[6]
#define OSTAT 0177564
#define ODATA 0177566

///////////////////////////////////
//////// базовые функции //////////
///////////////////////////////////

byte b_read  (adr a);            // читает из "старой памяти" mem байт с "адресом" a.
void b_write (adr a, byte val);  // пишет значение val в "старую память" mem в байт с "адресом" a.
word w_read  (adr a);            // читает из "старой памяти" mem слово с "адресом" a.
void w_write (adr a, word val);  // пишет значение val в "старую память" mem в слово с "адресом" a.

word w_read  (adr a) {

    return ((b_read(a + 1) << 8) | (b_read(a) & 0xFF));

}

byte b_read  (adr a) {
    return mem[a];
}

void b_write (adr a, byte val) {

    if(a == ODATA) {
        fprintf(stderr, "%c", val);
    }
    else
        mem[a] = val;
}

void w_write (adr a, word val) {
    //assert(a % 2 == 0);
    if(a == ODATA) {
        fprintf(stderr, "%c", val);
        return;
    }

    mem[a] = (byte)(val % 256);
    mem[a + 1] = (byte)(val / 256);
}

#define SIGN(w, is_byte) (is_byte ? ((w)>>7)&1 : ((w)>>15)&1 )

word byte_to_word(byte b) {

    word w;
    if (SIGN(b, 1) == 0) {
        w = 0;
        w |= b;
    }
    else {
        w = ~0xFF;
        w |= b;
    }
    return w;
}

void mem_dump(adr start, word n) {

    int i = 0;

    printf(";;;;;;;;mem_dump;;;;;;;;\n");
    for (i = 0; i <= n; i += 2) {
        printf("%06o : %06o\n", start + i , w_read(start + i));
    }
    printf(";;;;;;;;mem_dump;;;;;;;;\n");

}

void load_file(const char * filename) {

    FILE * fin = fopen(filename, "r");

    if (fin == NULL) {
        perror(filename);
        exit(1);
    }

    unsigned int val, n, adr;

    int i = 0;

    while (fscanf(fin, "%x%x", &adr, &n) == 2) {

        for(i = 0; i < n; i++) {
            fscanf(fin, "%x", &val);
            b_write(adr + i, val);
        }
    }


    mem_dump(01000, n);

    fclose(fin);
}

void test_mem() {

    printf(";;;;;;;;testing;;;;;;;;\n");

    byte b0, b1;
    word w0, w;

    w0 = 0x0a0b;
    w_write(0, w0);
    w = w_read(0);
    printf("%04hx\n%04hx\n", w0, w);
    assert (w0 == w);

    w0 = 0x0a0b;
    w_write(2, w0);
    b0 = b_read(2);
    b1 = b_read(3);
    printf("%04hx\n%02hhx%02hhx\n", w0, b1, b0);
    assert(b0 == (byte)0x0b);
    assert(b1 == (byte)0x0a);

    w0 = 0xfafb;
    w_write(14, w0);
    b0 = b_read(14);
    b1 = b_read(15);
    printf("%04hx\n%02hhx%02hhx\n", w0, b1, b0);
    assert(b0 == (byte)0xfb);
    assert(b1 == (byte)0xfa);

    b1 = 0x0A;
    b0 = 0x0B;
    b_write(4, b0);
    b_write(5, b1);
    w = w_read(4);
    printf("%04hx\n%02hhx%02hhx\n", w, b1, b0);
    assert(w == 0x0A0B);

    b1 = 0x0A;
    b0 = 0x0B;
    b_write(7, b1);
    b_write(6, b0);
    w = w_read(6);
    printf("%04hx\n%02hhx%02hhx\n", w, b1, b0);
    assert(w == 0x0A0B);

    b1 = 0xfA;
    b0 = 0xfB;
    b_write(8, b0);
    b_write(9, b1);
    w = w_read(8);
    printf("%04hx\n%02hhx%02hhx\n", w, b1, b0);
    assert(w == 0xfAfB);

    b1 = 0xfA;
    b0 = 0xfB;
    b_write(11, b1);
    b_write(10, b0);
    w = w_read(10);
    printf("%04hx\n%02hhx%02hhx\n", w, b1, b0);
    assert(w == 0x0A0B);

    printf(";;;;;;;;testing;;;;;;;;\n\n");
}

void print_registers() {

    int i = 0;

    for(; i < 8; i += 2)
        printf("r%d = %06o   ", i, reg[i]);

    printf("\n");

    for(i = 1; i < 8; i += 2)
        printf("r%d = %06o   ", i, reg[i]);

    printf("\n\n");

}

////////////////////////////////////
///////// аргументы, флаги /////////
////////////////////////////////////

#define REG 0
#define MEM 1

struct Variable {
    word val;    // что
    adr a;       // куда
    word res;    // результат
    word space;  // где

};

struct Variable ss, dd, nn;

struct sign {
    char val;
    char sign;
};

struct sign xx;

struct Flag {
    char N;
    char Z;
    char V;
    char C;
};

struct Flag flag;

struct P_Command {
    int B;        // Byte
    word r1;      // 1 operand
    word r2;      // 2 operand
};

struct P_Command create_command(word w) {
    struct P_Command res;

    res.B = (w >> 15);
    res.r1 = (w >> 6) & 7;
    res.r2 = w & 7;
    return res;
}

void change_flag(struct P_Command PC) {

    if (PC.B) {
        flag.N = (dd.res >> 7) & 1;
    }
    else {
        flag.N = (dd.res >> 15) & 1;
    }
    flag.Z = (dd.res == 0);
}

struct Variable get_mr(word w, int b) {
    struct Variable res;

    int r = w & 7;  // w && 111
    int mode = (w >> 3) & 7;           // mmmrrr - можно получить мод и регистр

    word n;
    res.space = MEM;

    switch (mode) {

        case 0: // inc R5
            res.a = r;
            res.val = reg[r];
            printf("R%d ", r);
            res.space = REG;
            break;

        case 1: // inc (R5)
            res.a = reg[r];
            res.val = b ? b_read(res.a) : w_read(res.a);
            printf("(R%d) ", r);
            break;

        case 2: // inc (R5)+ | inc #nn
            res.a = reg[r];
            res.val = b ? b_read(res.a) : w_read(res.a);
            if (r == 7 || r == 6 || b == 0)   //зависит от типа инструкции : байтовая или пословная
                reg[r] += 2;
            else
                reg[r] ++;
            if (r == 7) {
                printf("#%o ", res.val);
            } else {
                printf("(R%o)+ ", r);
            }
            break;

        case 3: // inc @(R5)+ | inc @#nn
            res.a = reg[r];
            if (r == 7 || r == 6 || b == 0) {
                res.a = w_read ((adr) reg[r]);
                res.val = w_read ((adr) w_read ((adr) (reg[r])));
                printf ("@#%o", w_read((adr) (reg[r])));
                reg[r] += 2;
            }
            else {
                res.a = w_read ((adr) reg[r]);
                res.val = b_read ((adr) w_read ((adr) (reg[r])));
                reg[r] += 2;       // reg[r] ++;
                printf ("@(R%o)+", r);
            }
            break;

        case 4: // dec -(R5)
            if (r == 7 || r == 6 || b == 0) {
                reg[r] -= 2;
                res.a = reg[r];
                res.val = w_read (res.a);
            }
            else {
                reg[r] --;
                res.a = reg[r];
                res.val = b_read (res.a);
            }
            printf("-(R%d) ", r);
            break;

        case 5: // dec @-(R5)

            printf ("@-(R%o)", r);
            reg[r] -= 2;
            res.a = reg[r];
            res.a = w_read (res.a);
            res.val = w_read (res.a);
            break;

        case 6: //
            n = w_read(pc);
            pc += 2;
            res.a = reg[r] + n;
            res.val = w_read(res.a);        // TODO byte cmd
            if (r==7)
                printf("%o ", res.a);
            else
                printf("%o(r%d) ", n, r);
            break;

        default:
            printf("Mode %d not implemented yet!\n", mode);
            exit(1);
    }
    return res;
}

struct Variable get_nn(word w) {
    struct Variable res;

    res.a = (w >> 6) & 07;
    res.val = w & 077;

    printf("R%d, %o", reg[nn.a], pc - 2 * nn.val);

    return res;
}

struct sign get_xx(word w) {
    struct sign res;

    res.val = w & 0xFF;

    unsigned int x = pc + 2 * res.val;
    printf("%06o", x);

    return res;
}

////////////////////////////////////
////// арифмитические функции //////
////////////////////////////////////

void do_halt(struct P_Command PC) {
    printf("\n\n---------------------- halted -----------------------\n");
    print_registers();
    exit(0);
}

void do_mov(struct P_Command PC) {

    dd.res = ss.val;
    if(dd.space == REG)
        reg[dd.a]= dd.res;
    else
        w_write(dd.a, dd.res);

    change_flag(PC);

}

void do_add(struct P_Command PC) {

    dd.res = dd.val + ss.val;
    if (dd.space == REG) {
        reg[dd.a] = dd.res;
    }
    else {
        w_write(dd.a, dd.res);
    }
    change_flag(PC);

}

void do_sob(struct P_Command PC) {

    if (--reg[nn.a] != 0)
        pc -= 2 * nn.val;

}

void do_br(struct P_Command PC) {
    pc += 2 * xx.val;
}

void do_beq(struct P_Command PC) {
    if (flag.Z == 1)
        do_br(PC);
}

void do_movb(struct P_Command PC) {

    dd.res = ss.val;
    if (dd.space == REG)
        reg[dd.a] = byte_to_word(dd.res);
    else
        b_write(dd.a, (byte)dd.res);
    change_flag(PC);

}

void do_clr(struct P_Command PC) {

    dd.res = dd.val = 0;
    if(dd.space == REG)
        reg[dd.a] = dd.res;
    else
        w_write(dd.a, dd.res);

    flag.N = 0;
    flag.Z = 1;
    change_flag(PC);
}

void do_tstb(struct P_Command PC) {

    dd.res = dd.val;
    change_flag(PC);
}

void do_tst(struct P_Command PC) {
    dd.res = dd.val;
    change_flag(PC);
}

void do_bpl(struct P_Command PC) {
    if(flag.N == 0)
        do_br(PC);
}

void do_dec(struct P_Command PC) {
    dd.val--;
    change_flag(PC);
    if (dd.space == REG)
        reg[dd.a] = dd.val;
    else
        w_write(dd.a, dd.val);
}

void do_jsr(struct P_Command PC) {

    w_write(sp, reg[PC.r1]);
    sp -= 2;
    reg[PC.r1] = pc;
    pc = dd.a;

}

void do_rts(struct P_Command PC) {

    pc = reg[PC.r2];
    sp += 2;
    reg[PC.r2] = w_read(reg[6]);
}

//////////////////////////////////////
//// структура итоговой программы ////
//////////////////////////////////////

#define NO_PARAM	0
#define HAS_SS		1
#define HAS_DD		2
#define HAS_NN		4
#define HAS_XX		8

struct Command {
    word mask;
    word opcode;
    const char * name;
    void (*do_func)(struct P_Command PC);
    word param;
};

const struct Command commands[] = {
        {0170000, 0010000, "mov",  do_mov,  HAS_SS},
        {0170000, 0060000, "add",  do_add,  HAS_SS},
        {0177000, 0077000, "sob",  do_sob,  HAS_NN},
        {0017700, 0005000, "clr",  do_clr,  HAS_DD},
        {0xFF00,  0000400, "br",   do_br,   HAS_XX},
        {0xFF00,  0001400, "beq",  do_beq,  HAS_XX},
        {0170000, 0110000, "movb", do_movb, HAS_SS},
        {0177700, 0105700, "tstb", do_tstb, HAS_DD},
        {0xFF00,  0100000, "bpl",  do_bpl,  HAS_XX},
        {0177700, 0005700, "tst",  do_tst,  HAS_DD},
        {0177700, 0005300, "dec",  do_dec,  HAS_DD},
        {0177000, 0004000, "jsr",  do_jsr,  HAS_DD},
        {0177770, 0000200, "rts",  do_rts,  HAS_DD},
        {0xFFFF,  0,       "halt", do_halt, NO_PARAM},
};

void run_programm() {

    mem[OSTAT] |= 128;
    pc = 01000;
    word w;

    printf("\n---------------------- running ----------------------\n");

    while(1) {
        w = w_read(pc);
//        printf("%06o : %06o ", pc, w);  // отладочная печать
        printf("%06o : ", pc);            // нормальная печать

        pc += 2;
        struct P_Command PC = create_command(w);
        int size = sizeof(commands)/sizeof(struct Command);

        for (int j = 0; j < size; j++)
            if ((commands[j].mask & w) == commands[j].opcode) {
                printf("%s    ", commands[j].name);

                if (commands[j].param == HAS_SS) {
                    ss = get_mr(w >> 6, PC.B);
                    dd = get_mr(w, PC.B);
//                    printf("\nss = %o, %o\n", ss.val, ss.a);
//                    printf("dd = %o, %o\n", dd.val, dd.a);
                }
                if (commands[j].param == HAS_DD) {
                    dd = get_mr(w, PC.B);
//                    printf("\ndd = %o, %o\n", dd.val, dd.a);
                }
                if (commands[j].param == HAS_NN) {
                    nn = get_nn(w);
//                    printf("\nnn = %o, %o\n", nn.val, nn.a);
                }
                if (commands[j].param == HAS_XX) {
                    xx = get_xx(w);
//                    printf("\nxx = %o\n", xx.val);
                }
                commands[j].do_func(PC);
            }

        printf("\n");

//        print_registers();

    }
}

/////////////////////////////////////
/////////////// main ////////////////
/////////////////////////////////////

int main(int argc, char * argv[]) {

//    test_mem();

    mem[OSTAT] = -1;

    if (argc == 1) {           // проверка, что agrv[1] есть
        printf("USAGE: %s filename\n", argv[0]);
        exit(1);
    }

    load_file(argv[1]);

    run_programm();

    return 0;
}
