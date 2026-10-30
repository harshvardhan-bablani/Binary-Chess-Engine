#pragma once

// Interactive game against the engine: you type moves in normal notation
// (e4, Nf3, exd5, O-O), the engine answers immediately.
// `human_is_white` chooses which side you play.
void play_mode(bool human_is_white);
