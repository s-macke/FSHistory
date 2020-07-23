void disasmout(unsigned seg, unsigned ofs, int count) {
    char buffer[512];
    for (int i = 0; i < count; i++) {
        unsigned newofs = disasm(seg, ofs, ram, buffer);
        printf("0x%04x:0x%04x   %s\n", seg, ofs, buffer);
        if (buffer[3] == 0 || buffer[2] == ':') {
            newofs = disasm(seg, newofs, ram, buffer);
            printf("0x%04x:0x%04x   %s\n", seg, ofs, buffer);
        }
        if (buffer[0] == 'r' && buffer[1] == 'e'  && buffer[2] == 'p') {
            newofs = disasm(seg, newofs, ram, buffer);
            printf("0x%04x:0x%04x   %s\n", seg, ofs, buffer);
        }
        ofs = newofs;
    }
}

void PrintStatus() {
    printf("=================================\n");
    printf("Status of CPU\n");
    printf("cs:ip : 0x%04x:0x%04x\n", segregs[regcs], ip);
    printf("cs:ip : 0x%04x:0x%04x (last ip)\n", segregs[regcs], saveip);
    printf("ss:esp : 0x%04x:0x%08x\n", segregs[regss], regs.dwordregs[regesp]);
    printf("ds:esi : 0x%04x:0x%08x\n", segregs[regds], regs.dwordregs[regesi]);
    printf("es:edi : 0x%04x:0x%08x\n", segregs[reges], regs.dwordregs[regedi]);
    printf("eax : 0x%08x\n", regs.dwordregs[regeax]);
    printf("ebx : 0x%08x\n", regs.dwordregs[regebx]);
    printf("ecx : 0x%08x\n", regs.dwordregs[regecx]);
    printf("edx : 0x%08x\n", regs.dwordregs[regedx]);
    printf("ebp : 0x%08x\n", regs.dwordregs[regebp]);
    printf("eflags : 0x%08x\n", makeflags());
    printf("cf : %i ", cf);
    printf("pf : %i ", pf);
    printf("af : %i ", af);
    printf("zf : %i ", zf);
    printf("sf : %i ", sf);
    printf("tf : %i ", tf);
    printf("ifl : %i ", ifl);
    printf("df : %i ", df);
    printf("of : %i\n", of);
    printf("ntf : %i\n", ntf);
    printf("ioplf : %i\n", ioplf);

    printf("=================================\n");
    disasmout(segregs[regcs], saveip, 3);
    printf("=================================\n");
}

void setcsip(uint16_t _cs, uint16_t _ip) {
    segregs[regcs] = _cs;
    ip = _ip;
}

void getcsip(uint16_t *_cs, uint16_t *_ip) {
    *_cs = segregs[regcs];
    *_ip = ip;
}

void setsssp(uint16_t _ss, uint16_t _sp) {
    segregs[regss] = _ss;
    regs.wordregs[regsp] = _sp;
}

void SetHLTstate() {
    //printf("Enter HLT State\n");
    hltstate = 1;
    ifl = 0;
}

void ClearHLTstate() {
    hltstate = 0;
}