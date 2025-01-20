#include <cstdio>
#include <cstdlib>
#include <raylib.h>
#define NO_FONT_AWESOME

#include "bridge.h"
#include "imgui.h"
#include "rlImGui.h"
#include <string>

addressing_mode addr_mode_lookup[] = {
    IMPLIED,     INDIRECTX, IMPLIED,     IMPLIED,   IMPLIED,     ZEROPAGE,
    ZEROPAGE,    IMPLIED,   IMPLIED,     IMMEDIATE, ACCUMULATOR, IMPLIED,
    ABSOLUTE,    ABSOLUTE,  ABSOLUTE,    IMPLIED,   RELATIVE,    INDIRECTY,
    IMPLIED,     IMPLIED,   IMPLIED,     ZEROPAGEX, ZEROPAGEX,   IMPLIED,
    IMPLIED,     ABSY,      IMPLIED,     IMPLIED,   IMPLIED,     ABSX,
    ABSX,        IMPLIED,   ABSOLUTE,    INDIRECTX, IMPLIED,     IMPLIED,
    ZEROPAGE,    ZEROPAGE,  ZEROPAGE,    IMPLIED,   IMPLIED,     IMMEDIATE,
    ACCUMULATOR, IMPLIED,   ABSOLUTE,    ABSOLUTE,  ABSOLUTE,    IMPLIED,
    RELATIVE,    INDIRECTY, IMPLIED,     IMPLIED,   IMPLIED,     ZEROPAGEX,
    ZEROPAGEX,   IMPLIED,   IMPLIED,     ABSY,      IMPLIED,     IMPLIED,
    IMPLIED,     ABSX,      ABSX,        IMPLIED,   IMPLIED,     INDIRECTX,
    IMPLIED,     IMPLIED,   IMPLIED,     ZEROPAGE,  ZEROPAGE,    IMPLIED,
    IMPLIED,     IMMEDIATE, ACCUMULATOR, IMPLIED,   ABSOLUTE,    ABSOLUTE,
    ABSOLUTE,    IMPLIED,   RELATIVE,    INDIRECTY, IMPLIED,     IMPLIED,
    IMPLIED,     ZEROPAGEX, ZEROPAGEX,   IMPLIED,   IMPLIED,     ABSY,
    IMPLIED,     IMPLIED,   IMPLIED,     ABSX,      ABSX,        IMPLIED,
    IMPLIED,     INDIRECTX, IMPLIED,     IMPLIED,   IMPLIED,     ZEROPAGE,
    ZEROPAGE,    IMPLIED,   IMPLIED,     IMMEDIATE, ACCUMULATOR, IMPLIED,
    INDIRECT,    ABSOLUTE,  ABSOLUTE,    IMPLIED,   RELATIVE,    INDIRECTY,
    IMPLIED,     IMPLIED,   IMPLIED,     ZEROPAGEX, ZEROPAGEX,   IMPLIED,
    IMPLIED,     ABSY,      IMPLIED,     IMPLIED,   IMPLIED,     ABSX,
    ABSX,        IMPLIED,   IMPLIED,     INDIRECTX, IMPLIED,     IMPLIED,
    ZEROPAGE,    ZEROPAGE,  ZEROPAGE,    IMPLIED,   IMPLIED,     IMPLIED,
    IMPLIED,     IMPLIED,   ABSOLUTE,    ABSOLUTE,  ABSOLUTE,    IMPLIED,
    RELATIVE,    INDIRECTY, IMPLIED,     IMPLIED,   ZEROPAGEX,   ZEROPAGEX,
    ZEROPAGEX,   ZEROPAGEY, IMPLIED,     ABSY,      IMPLIED,     IMPLIED,
    IMPLIED,     ABSX,      IMPLIED,     IMPLIED,   IMMEDIATE,   INDIRECTX,
    IMMEDIATE,   IMPLIED,   ZEROPAGE,    ZEROPAGE,  ZEROPAGE,    IMPLIED,
    IMPLIED,     IMMEDIATE, IMPLIED,     IMPLIED,   ABSOLUTE,    ABSOLUTE,
    ABSOLUTE,    IMPLIED,   RELATIVE,    INDIRECTY, IMPLIED,     IMPLIED,
    ZEROPAGEX,   ZEROPAGEX, ZEROPAGEY,   IMPLIED,   IMPLIED,     ABSY,
    IMPLIED,     IMPLIED,   ABSX,        ABSX,      ABSY,        IMPLIED,
    IMMEDIATE,   INDIRECTX, IMPLIED,     IMPLIED,   ZEROPAGE,    ZEROPAGE,
    ZEROPAGE,    IMPLIED,   IMPLIED,     IMMEDIATE, IMPLIED,     IMPLIED,
    ABSOLUTE,    ABSOLUTE,  ABSOLUTE,    IMPLIED,   RELATIVE,    INDIRECTY,
    IMPLIED,     IMPLIED,   IMPLIED,     ZEROPAGEX, ZEROPAGEX,   IMPLIED,
    IMPLIED,     ABSY,      IMPLIED,     IMPLIED,   IMPLIED,     ABSX,
    ABSX,        IMPLIED,   IMMEDIATE,   INDIRECTX, IMPLIED,     IMPLIED,
    ZEROPAGE,    ZEROPAGE,  ZEROPAGE,    IMPLIED,   IMPLIED,     IMMEDIATE,
    IMPLIED,     IMPLIED,   ABSOLUTE,    ABSOLUTE,  ABSOLUTE,    IMPLIED,
    RELATIVE,    INDIRECTY, IMPLIED,     IMPLIED,   IMPLIED,     ZEROPAGEX,
    ZEROPAGEX,   IMPLIED,   IMPLIED,     ABSY,      IMPLIED,     IMPLIED,
    IMPLIED,     ABSX,      ABSX,        IMPLIED};
std::string addr_mode_format_lookup[] = {
    "#$%02x",    "$%04x",     "$%02x",   "",        "",
    "($%02x,X)", "($%02x),Y", "$%02x,X", "$%04x,X", "$%04x,Y",
    "$%04x",     "($%04x)",   "$%02x,Y"};
uint8_t addr_mode_extra_bytes_lookup[] = {1, 2, 1, 0, 0, 1, 1,
                                          1, 2, 2, 2, 2, 1};
std::string opcode_lookup[] = {
    "BRK", "ORA", "",    "", "",    "ORA", "ASL", "", "PHP", "ORA", "ASL", "",
    "",    "ORA", "ASL", "", "BPL", "ORA", "",    "", "",    "ORA", "ASL", "",
    "CLC", "ORA", "",    "", "",    "ORA", "ASL", "", "JSR", "AND", "",    "",
    "BIT", "AND", "ROL", "", "PLP", "AND", "ROL", "", "BIT", "AND", "ROL", "",
    "BMI", "AND", "",    "", "",    "AND", "ROL", "", "SEC", "AND", "",    "",
    "",    "AND", "ROL", "", "RTI", "EOR", "",    "", "",    "EOR", "LSR", "",
    "PHA", "EOR", "LSR", "", "JMP", "EOR", "LSR", "", "BVC", "EOR", "",    "",
    "",    "EOR", "LSR", "", "CLI", "EOR", "",    "", "",    "EOR", "LSR", "",
    "RTS", "ADC", "",    "", "",    "ADC", "ROR", "", "PLA", "ADC", "ROR", "",
    "JMP", "ADC", "ROR", "", "BVS", "ADC", "",    "", "",    "ADC", "ROR", "",
    "SEI", "ADC", "",    "", "",    "ADC", "ROR", "", "",    "STA", "",    "",
    "STY", "STA", "STX", "", "DEY", "",    "TXA", "", "STY", "STA", "STX", "",
    "BCC", "STA", "",    "", "STY", "STA", "STX", "", "TYA", "STA", "TXS", "",
    "",    "STA", "",    "", "LDY", "LDA", "LDX", "", "LDY", "LDA", "LDX", "",
    "TAY", "LDA", "TAX", "", "LDY", "LDA", "LDX", "", "BCS", "LDA", "",    "",
    "LDY", "LDA", "LDX", "", "CLV", "LDA", "TSX", "", "LDY", "LDA", "LDX", "",
    "CPY", "CMP", "",    "", "CPY", "CMP", "DEC", "", "INY", "CMP", "DEX", "",
    "CPY", "CMP", "DEC", "", "BNE", "CMP", "",    "", "",    "CMP", "DEC", "",
    "CLD", "CMP", "",    "", "",    "CMP", "DEC", "", "CPX", "SBC", "",    "",
    "CPX", "SBC", "INC", "", "INX", "SBC", "NOP", "", "CPX", "SBC", "INC", "",
    "BEQ", "SBC", "",    "", "",    "SBC", "INC", "", "SED", "SBC", "",    "",
    "",    "SBC", "INC", ""};

extern "C" {
void cpp_init(void) { rlImGuiSetup(true); }

void cpp_imGui_render(cpu_t *cpu, ppu_t *ppu, apu_t *apu) {
    rlImGuiBegin();

    float old_size = ImGui::GetFont()->Scale;
    ImGui::GetFont()->Scale *= 2;
    ImGui::PushFont(ImGui::GetFont());

    ImGui::Begin("debug", NULL,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    ImGui::Text("PC: 0x%04x", cpu->pc);
    ImGui::SameLine();
    if (ImGui::Button("Start")) {
        cpu->run = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        cpu->run = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        cpu->step = true;
    }
    ImGui::SameLine();
    uint8_t opcode = read_8(cpu->pc);
    addressing_mode mode = addr_mode_lookup[opcode];
    std::string extra;
    switch (addr_mode_extra_bytes_lookup[mode]) {
    case 0:
        extra = addr_mode_format_lookup[mode];
        break;
    case 1:
        extra = std::string(TextFormat(addr_mode_format_lookup[mode].c_str(),
                                       read_8(cpu->pc + 1)));
        break;
    case 2:
        extra = std::string(TextFormat(addr_mode_format_lookup[mode].c_str(),
                                       read_16le(cpu->pc + 1)));
        break;
    }
    ImGui::Text("Next: %s %s", opcode_lookup[opcode].c_str(), extra.c_str());

    static char break_point_buffer[7];
    ImGui::InputText("Breakpoint", break_point_buffer, 7);
    cpu->run_until = strtol(break_point_buffer, NULL, 16);
    ImGui::NewLine();
    static char address_buffer[5];
    ImGui::InputText("RAM Address", address_buffer, 5);
    static char value_buffer[3];
    ImGui::InputText("Value", value_buffer, 3);
    if (ImGui::Button("Set")) {
        cpu->memory.ram[strtol(address_buffer, NULL, 16)] =
            strtol(value_buffer, NULL, 16);
    }

    static int32_t page = 0;
    ImGui::InputInt("Page", &page);
    if (ImGui::CollapsingHeader("RAM page")) {
        ImGui::BeginTable("rampage", 8,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        for (int i = 0; i < 32; i++) {
            for (int j = 0; j < 8; j++) {
                ImGui::TableNextColumn();
                ImGui::Text("0x%02x",
                            cpu->memory.ram[page * 0x100 + i * 8 + j]);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("0x%02x", page * 0x100 + i * 8 + j);
                }
            }
            if (i != 31)
                ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }

    if (ImGui::CollapsingHeader("CPU")) {
        ImGui::Text("A: 0x%02x X: 0x%02x Y: 0x%02x SP: 0x%02x P: 0x%02x", cpu->a, cpu->x,
                    cpu->y, cpu->sp, cpu->p);

        ImGui::Text("Status: NV.BDIZC");
        ImGui::Text("        %d%d%d%d%d%d%d%d", (cpu->p & 0x80) != 0,
                    (cpu->p & 0x40) != 0, (cpu->p & 0x20) != 0,
                    (cpu->p & 0x10) != 0, (cpu->p & 0x8) != 0,
                    (cpu->p & 0x4) != 0, (cpu->p & 0x2) != 0,
                    (cpu->p & 0x1) != 0);

        ImGui::Text("Elapsed cycles: %lu", cpu->elapsed_cycles);
        ImGui::Text("Remaining cycles: %lf", cpu->remaining_cycles);

        ImGui::Text("JOY1: 0x%02x (%s)", ppu->joy1_mirror, GetGamepadName(0));
    }

    if (ImGui::CollapsingHeader("PPU")) {
        ImGui::Text("Scroll X: %d, Scroll Y: %d", ppu->scroll_x, ppu->scroll_y);
        ImGui::Text("Base Nametable: %d", ppu->base_nametable_address);

        float new_old_size = ImGui::GetFont()->Scale;
        ImGui::GetFont()->Scale /= 8;
        ImGui::PushFont(ImGui::GetFont());

        ImGui::GetFont()->Scale = new_old_size;
        ImGui::PopFont();
    }

    if(ImGui::CollapsingHeader("APU")) {
        ImGui::Text("Channel 1 volume: %f", apu->channel1.envelope_level / 15.f);
        ImGui::Text("Channel 1 envelope enable: %s", !apu->channel1.constant_volume ? "true" : "false");
        ImGui::Text("Channel 1 loop: %s", apu->channel1.loop ? "true" : "false");
        ImGui::Text("Channel 1 envelope speed: %d", apu->channel1.envelope_decay_counter);
        ImGui::Text("Channel 1 frequency: %f", apu->channel1.frequency);
        ImGui::Text("Channel 1 length timer: %d", apu->channel1.length_counter);
        ImGui::NewLine();
        ImGui::Text("Channel 2 volume: %f", apu->channel2.envelope_level / 15.f);
        ImGui::Text("Channel 2 envelope enable: %s", !apu->channel2.constant_volume ? "true" : "false");
        ImGui::Text("Channel 2 loop: %s", apu->channel2.loop ? "true" : "false");
        ImGui::Text("Channel 2 envelope speed: %d", apu->channel2.envelope_decay_counter);
        ImGui::Text("Channel 2 frequency: %f", apu->channel2.frequency);
        ImGui::Text("Channel 2 length timer: %d", apu->channel2.length_counter);
    }

    if (ImGui::CollapsingHeader("VRAM")) {
        ImGui::BeginTable("vram", 8,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        for (int i = 0; i < 0x800 / 8; i++) {
            for (int j = 0; j < 8; j++) {
                ImGui::TableNextColumn();
                ImGui::Text("0x%02x",
                            ppu->memory.cart.read(0x2000 + j + i * 8));
            }
            if (i != 0x1000 / 8 - 1)
                ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }

    if (ImGui::CollapsingHeader("OAM")) {
        ImGui::BeginTable("oam", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        for (int i = 0; i < 0x100 / 4; i++) {
            ImGui::TableNextColumn();
            ImGui::Text("0x%02x", ppu->oam[i].y_pos);
            ImGui::TableNextColumn();
            ImGui::Text("0x%02x", ppu->oam[i].tile_index);
            ImGui::TableNextColumn();
            ImGui::Text("0x%02x", ppu->oam[i].attributes);
            ImGui::TableNextColumn();
            ImGui::Text("0x%02x", ppu->oam[i].x_pos);
            if (i != 0x100 / 4 - 1)
                ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }

    if (ImGui::CollapsingHeader("Palette Memory")) {
        ImGui::BeginTable("palettememory", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        for (int i = 0; i < 0x20; i += 4) {
            ImGui::TableNextColumn();
            ImGui::Text("0x%02x", ppu->memory.palette_ram[i]);
            ImGui::TableNextColumn();
            ImGui::Text("0x%02x", ppu->memory.palette_ram[i + 1]);
            ImGui::TableNextColumn();
            ImGui::Text("0x%02x", ppu->memory.palette_ram[i + 2]);
            ImGui::TableNextColumn();
            ImGui::Text("0x%02x", ppu->memory.palette_ram[i + 3]);
            if (i != 0x20 / 4 - 1)
                ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }

    ImGui::End();

    ImGui::GetFont()->Scale = old_size;
    ImGui::PopFont();

    rlImGuiEnd();
}

void cpp_end(void) { rlImGuiShutdown(); }
}
