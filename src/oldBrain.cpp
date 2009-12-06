/*
 *  oldBrain.cpp
 *  openc2e
 *
 *  Created by Alyssa Milburn on Mon Aug 13 2007.
 *  Copyright (c) 2007 Alyssa Milburn. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 */

#include "oldBrain.h"
#include "Creature.h"

/*
 * svrule examples:
 *
 * * creatures 1:
 * output:TRUE:input
 *
 * * creatures 2:
 * random:0:chem 5:PLUS:state
 * state:PLUS:type 0:MINUS:type 1
 * input:TRUE:output:TRUE:suscept:move twrds:255:64
 * suscept:TRUE:chem 0:TRUE:STW
 *
 * * canny:
 * state:PLUS:state:PLUS:state
 * type 0:TRUE:type 0:PLUS:type 1
 *
 */

void oldSVRule::init(uint8 version, uint8 *src) {
	length = (version == 0) ? 8 : 12;
	for (unsigned int i = 0; i < length; i++) {
		rules[i] = src[i];
		if (version == 0 && rules[i] > 21) // if rule is above the strength id
			rules[i] += 8; // then skip the 8 new C2 svrules (to map to C2 svrules)
	}
	rndconst = 0; // TODO: correct?
}

unsigned char oldLobe::evaluateSVRuleConstant(oldNeuron *cell, oldDendrite *dend, uint8 id, oldSVRule &rule) {
	switch (id) {
		/*
		 * these numbers are the C2 svrules (see rewrite in oldSVRule constructor)
		 */
		case 1: // 0
			return 0;

		case 2: // 1
			return 1;

		case 3: // 64
			return 64;

		case 4: // 255
			return 255;

		case 5: // chem0
			return chems[0];

		case 6: // chem1
			return chems[1];

		case 7: // chem2
			return chems[2];

		case 8: // chem3
			return chems[3];

		case 9: // state
			return cell->state;

		case 10: // output
			return cell->output;

		case 11: // thres
			return threshold;

		case 12: // type0
			return dendrite_sum(0, false);

		case 13: // type1
			return dendrite_sum(1, false);

		case 14: // anded0
			return dendrite_sum(0, true);

		case 15: // anded1
			// unused?
			return dendrite_sum(1, true);

		case 16: // input
			// This comes from IMPT for the decision lobe.
			if (!dend) return 0;
			return dend->src->output;

		case 17: // conduct
			// unused?
			if (!dend) return 0;
			return 0; // TODO: what's this?

		case 18: // suscept
			if (!dend) return 0;
			return dend->suscept;

		case 19: // STW
			if (!dend) return 0;
			return dend->stw;

		case 20: // LTW
			// unused?
			if (!dend) return 0;
			return dend->ltw;

		case 21: // strength
			// unused?
			if (!dend) return 0;
			return dend->strength;

		case 22: // 32
			// unused?
			return 32;

		case 23: // 128
			// unused?
			return 128;

		case 24: // rnd const
			// unused?
			return rule.rndconst;

		case 25: // chem4
			return chems[4];

		case 26: // chem5
			return chems[5];

		case 27: // leak in
			// unused: back/forward prop
			return cell->leakin;

		case 28: // leak out
			// unused: back/forward prop
			return cell->leakout;

		case 29: // curr src leak in
			// unused: back/forward prop
			if (!dend) return 0;
			return dend->src->leakin;

		default:
			return 0;
	}
}

unsigned char oldLobe::processSVRule(oldNeuron *cell, oldDendrite *dend, oldSVRule &rule) {
	unsigned char state = 0;

	// original engine seems to simply happily walk off the end of the svrule array for constants!
	// so our behaviour for the 'if (i == len)' lines is NOT the same

	for (unsigned int i = 0; i < rule.length; i++) {
		switch (rule.rules[i]) {
			case 0: // <end>
				return state;

			default:
				state = evaluateSVRuleConstant(cell, dend, rule.rules[i], rule);
				break;

			case 30: // TRUE
				if (!state) return 0;
				break;

			case 31: // PLUS
				i++;
				if (i == rule.length) return state;
				state = state + evaluateSVRuleConstant(cell, dend, rule.rules[i], rule);
				break;

			case 32: // MINUS
				i++;
				if (i == rule.length) return state;
				state = state - evaluateSVRuleConstant(cell, dend, rule.rules[i], rule);
				break;

			case 33: // TIMES
				// unused?
				i++;
				if (i == rule.length) return state;
				state = (state * evaluateSVRuleConstant(cell, dend, rule.rules[i], rule)) / 256;
				break;

			case 34: // INCR
				// unused?
				state++;
				break;

			case 35: // DECR
				// unused?
				state--;
				break;

			case 36: // FALSE
				if (state) return 0;
				break;

			case 37: // multiply
				// unused?
				i++;
				if (i == rule.length) return state;
				state = state * evaluateSVRuleConstant(cell, dend, rule.rules[i], rule);
				break;

			case 38: // average
				// unused?
				i++;
				if (i == rule.length) return state;
				state = (state + evaluateSVRuleConstant(cell, dend, rule.rules[i], rule)) / 2;
				break;

			case 39: { // move twrds
				i++;
				if (i == rule.length) return state;
				unsigned char towards = evaluateSVRuleConstant(cell, dend, rule.rules[i], rule);
				i++;
				if (i == rule.length) return state;
				unsigned char multiplier = evaluateSVRuleConstant(cell, dend, rule.rules[i], rule);
				state = ((towards - state) * multiplier) / 256;
				} break;

			case 40: { // random
				i++;
				if (i == rule.length) return state;
				unsigned char min = evaluateSVRuleConstant(cell, dend, rule.rules[i], rule);
				i++;
				if (i == rule.length) return state;
				unsigned char max = evaluateSVRuleConstant(cell, dend, rule.rules[i], rule);
				state = (rand() % (max - min + 1)) + min;
				} break;
		}
	}

	return state;
}

unsigned char oldLobe::dendrite_sum(unsigned int type, bool only_if_firing) {
	// TODO: cache this result, since it will be used every time the svrule runs
	// (and remember you can calculate both only_if_firing and not only_if_firing in one go!)
	unsigned int sum = 0; // TODO: sum((src output * strength) / 255)
	return (sum * inputgain) / 255;
}

oldLobe::oldLobe(oldBrain *b, oldBrainLobeGene *g) {
	assert(b);
	parent = b;
	assert(g);
	ourGene = g;

	inited = false;

	staterule.init(g->version(), (uint8 *)g->staterule);

	for (unsigned int i = 0; i < 2; i++) {
		oldDendriteInfo *dend_info = &g->dendrite1;
		if (i == 1) dend_info = &g->dendrite2;

		strgainrule[i].init(g->version(), (uint8 *)dend_info->strgainrule);
		strlossrule[i].init(g->version(), (uint8 *)dend_info->strlossrule);
		susceptrule[i].init(g->version(), (uint8 *)dend_info->susceptrule);
		relaxrule[i].init(g->version(), (uint8 *)dend_info->relaxrule);

		if (g->version() == 1) { // back/forward propogation is C2 only
			backproprule[i].init(g->version(), (uint8 *)dend_info->backproprule);
			forproprule[i].init(g->version(), (uint8 *)dend_info->forproprule);
		} else {
			backproprule[i].length = 0;
			forproprule[i].length = 0;
		}
	}

	threshold = g->nominalthreshold;
	leakagerate = g->leakagerate;
	inputgain = g->inputgain;

	width = g->width;
	height = g->height;
	// TODO: good?
	if (width < 1) width = 1;
	if (height < 1) height = 1;

	neurons.reserve(width * height);

	oldNeuron n;
	for (unsigned int i = 0; i < width * height; i++) {
		neurons.push_back(n);
	}

	// TODO

	for (unsigned int i = 0; i < 6; i++) {
		chems[i] = 0;
	}
}

void oldLobe::init() {
	inited = true;
	wipe();
}

void oldLobe::wipe() {
	for (unsigned int i = 0; i < neurons.size(); i++) {
		neurons[i].state = neurons[i].output = ourGene->reststate; // TODO: good?
	}
}

void oldLobe::tick() {
	for (unsigned int i = 0; i < neurons.size(); i++) {
		unsigned char out = processSVRule(&neurons[i], NULL, staterule);

		// apply leakage rate in order to settle at rest state
		if ((parent->getTicks() & parent->getParent()->calculateTickMask(leakagerate / 8)) == 0) {
			if (out > ourGene->reststate)
				out = ourGene->reststate + ((out - ourGene->reststate) * parent->getParent()->calculateMultiplier(leakagerate / 8)) / 65536;
			else
				out = ourGene->reststate;
		}

		neurons[i].state = out;

		if (out < threshold)
			out = 0;
		else
			out -= threshold;

		neurons[i].output = out;

		tickDendrites(i, 0);
		tickDendrites(i, 1);
	}

	// TODO: data copied to perception lobe (ourGene->perceptflag - not just true/false!)

	if (ourGene->flags & 1) {
		// winner takes all
		unsigned char bestvalue = 0;
		unsigned char *bestoutput = NULL;
		for (unsigned int i = 0; i < neurons.size(); i++) {
			if (neurons[i].output > bestvalue) {
				bestvalue = neurons[i].output;
				bestoutput = &neurons[i].output;
			}
			neurons[i].output = 0;
		}
		if (bestoutput)
			*bestoutput = bestvalue;
	}

	// TODO: migration
}

void oldLobe::tickDendrites(unsigned int id, unsigned int type) {
	oldDendriteInfo *dend_info = &ourGene->dendrite1;
	if (type == 1) dend_info = &ourGene->dendrite2;

	oldNeuron &dest = neurons[id];

	for (unsigned int i = 0; i < dest.dendrites[type].size(); i++) {
		unsigned char out;

		oldDendrite &dend = dest.dendrites[type][i];

		// recalculate suscept
		out = processSVRule(&dest, &dend, susceptrule[type]);
		if (out > dend.suscept) {
			dend.suscept = out;
		} else {
			// decay old suscept
			if ((parent->getTicks() & parent->getParent()->calculateTickMask(dend_info->relaxsuscept / 8)) == 0) {
				dend.suscept = (dend.suscept * parent->getParent()->calculateMultiplier(dend_info->relaxsuscept / 8)) / 65536;
			}
		}

		// recalculate reinforce (TODO: why do we call this relaxrule?)
		out = processSVRule(&dest, &dend, relaxrule[type]);
		unsigned char x = ((int)dend.suscept * (int)out) / 255;
		if (x && x < dend.stw - dend.ltw)
			dend.stw = dend.ltw + x;

		// STW relax
		if ((parent->getTicks() & parent->getParent()->calculateTickMask(dend_info->relaxSTW / 8)) == 0) {
			out = dend.ltw + ((out - dend.ltw) * parent->getParent()->calculateMultiplier(dend_info->relaxSTW / 8)) / 65536;
		}

		// LTW gain
		if (dend_info->LTWgainrate && (parent->getTicks() % dend_info->LTWgainrate) == 0) {
			dend.ltw++;
		}

		// strength gain (TODO: don't run if maxed?)
		if (dend_info->strgain && (parent->getTicks() % dend_info->strgain) == 0) {
			out = processSVRule(&dest, &dend, strgainrule[type]);
			// TODO: overflow?
			dend.strength += out;
		}

		// strength loss (TODO: don't run if zero?)
		if (dend_info->strloss && (parent->getTicks() % dend_info->strloss) == 0) {
			out = processSVRule(&dest, &dend, strlossrule[type]);
			// TODO: underflow?
			dend.strength -= out;
			// TODO: when strength is lost: also reset STW, LTW, +0/output/state on dest neuron
		}

		// TODO: back propogation (set leak in)
		// TODO: front propogation (set leak out)
	}
}

oldBrain::oldBrain(oldCreature *p) {
	assert(p);
	parent = p;

	ticks = 0;
}

void oldBrain::processGenes() {
	shared_ptr<genomeFile> genome = parent->getGenome();
	
	for (vector<gene *>::iterator i = genome->genes.begin(); i != genome->genes.end(); i++) {
		if (!parent->shouldProcessGene(*i)) continue;
		
		if (typeid(**i) == typeid(oldBrainLobeGene)) {
			oldBrainLobeGene *g = (oldBrainLobeGene *)*i;
			oldLobe *l = new oldLobe(this, g);
			lobes[lobes.size()] = l; // TODO: muh
		}
	}
}

void oldBrain::init() {
	for (std::map<unsigned int, oldLobe *>::iterator i = lobes.begin(); i != lobes.end(); i++) {
		if (!(*i).second->wasInited()) (*i).second->init();
	}
}

void oldBrain::tick() {
	for (std::map<unsigned int, oldLobe *>::iterator i = lobes.begin(); i != lobes.end(); i++) {
		(*i).second->tick();
	}

	ticks++;
}

oldLobe *oldBrain::getLobeByTissue(unsigned int id) {
	if (lobes.find(id) == lobes.end())
		return 0;

	return lobes[id];
}

/* vim: set noet: */
