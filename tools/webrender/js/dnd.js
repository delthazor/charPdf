/** @typedef {import('./data.js').CharacterData} CharacterData */

export const ABILITIES = [
    'strength',
    'dexterity',
    'constitution',
    'intelligence',
    'wisdom',
    'charisma',
];

export const SKILL_DEFS = [
    { key: 'acrobatics', label: 'Acrobatics', stat: 'dexterity' },
    { key: 'animalHandling', label: 'Animal Handling', stat: 'wisdom' },
    { key: 'arcana', label: 'Arcana', stat: 'intelligence' },
    { key: 'athletics', label: 'Athletics', stat: 'strength' },
    { key: 'deception', label: 'Deception', stat: 'charisma' },
    { key: 'history', label: 'History', stat: 'intelligence' },
    { key: 'insight', label: 'Insight', stat: 'wisdom' },
    { key: 'intimidation', label: 'Intimidation', stat: 'charisma' },
    { key: 'investigation', label: 'Investigation', stat: 'intelligence' },
    { key: 'medicine', label: 'Medicine', stat: 'wisdom' },
    { key: 'nature', label: 'Nature', stat: 'intelligence' },
    { key: 'perception', label: 'Perception', stat: 'wisdom' },
    { key: 'performance', label: 'Performance', stat: 'charisma' },
    { key: 'persuasion', label: 'Persuasion', stat: 'charisma' },
    { key: 'religion', label: 'Religion', stat: 'intelligence' },
    { key: 'sleightOfHand', label: 'Sleight of Hand', stat: 'dexterity' },
    { key: 'stealth', label: 'Stealth', stat: 'dexterity' },
    { key: 'survival', label: 'Survival', stat: 'wisdom' },
];

export function calcModFromStatVal(statVal) {
    return Math.floor((statVal - 10) / 2);
}

export function formatSignedInt(value) {
    if (value >= 0) {
        return '+' + String(value);
    }
    return String(value);
}

export function capitalizeFirst(text) {
    if (!text) {
        return '';
    }
    return text.charAt(0).toUpperCase() + text.slice(1);
}

export function formatClassName(classId) {
    return capitalizeFirst(classId);
}

export function getStatMod(character, statName) {
    const stats = character.stats || {};
    const val = stats[statName];
    if (typeof val !== 'number') {
        return 0;
    }
    return calcModFromStatVal(val);
}

export function getProfBonus(character) {
    return character.proficiencies?.bonus ?? 0;
}

export function getSkillValue(character, statName, skillKey) {
    const skills = character.proficiencies?.skills;
    if (!skills || !skills[statName]) {
        return 0;
    }
    const val = skills[statName][skillKey];
    return typeof val === 'number' ? val : 0;
}

export function getSkillBonus(character, statName, skillValue) {
    const statMod = getStatMod(character, statName);
    const profBonus = getProfBonus(character);
    return statMod + profBonus * skillValue;
}

export function getSaveBonus(character, statName) {
    const saves = character.proficiencies?.savingThrows || {};
    const profValue = typeof saves[statName] === 'number' ? saves[statName] : 0;
    return getStatMod(character, statName) + getProfBonus(character) * profValue;
}

export function getInitiative(character) {
    const bonus = character.stats?.initiativeBonus ?? 0;
    return getStatMod(character, 'dexterity') + bonus;
}

export function getPassivePerception(character) {
    const bonus = character.stats?.pPercBonus ?? 0;
    return 10 + getStatMod(character, 'wisdom') + bonus;
}

function isBaseTypeArmor(armor) {
    return typeof armor.ac?.base === 'number';
}

function isFixmodTypeArmor(armor) {
    return typeof armor.ac?.fixmod === 'number';
}

export function armorStatContribution(armorAc, statMod) {
    if (typeof armorAc.modcap !== 'number') {
        return statMod;
    }
    return Math.min(statMod, armorAc.modcap);
}

function sumFixmodBonuses(armors) {
    let total = 0;
    for (const armor of armors) {
        if (isFixmodTypeArmor(armor)) {
            total += armor.ac.fixmod;
        }
    }
    return total;
}

export function getAc(character) {
    const acBonus = character.stats?.acBonus ?? 0;
    const usedArmors = character.equipment?.used?.armors ?? [];

    const baseTypeArmors = usedArmors.filter(isBaseTypeArmor);
    if (baseTypeArmors.length > 1) {
        const charName = character.name || 'character';
        console.warn('Warning: multiple base-type armors for ' + charName + ', using last');
    }

    let bodyAc = 0;
    if (baseTypeArmors.length === 0) {
        bodyAc = 10 + getStatMod(character, 'dexterity') + acBonus;
    } else {
        const armor = baseTypeArmors[baseTypeArmors.length - 1];
        const ac = armor.ac;
        const statMod = getStatMod(character, ac.modstat || 'dexterity');
        bodyAc = ac.base + armorStatContribution(ac, statMod) + acBonus;
    }

    return bodyAc + sumFixmodBonuses(usedArmors);
}

export function getSpellSaveDc(character, castStat) {
    if (!castStat) {
        return null;
    }
    return 8 + getStatMod(character, castStat) + getProfBonus(character);
}

export function getSpellAttackBonus(character, castStat) {
    if (!castStat) {
        return null;
    }
    return getStatMod(character, castStat) + getProfBonus(character);
}

export function getHitDiceString(character) {
    const classes = character.classes || {};
    const parts = [];
    for (const classId of Object.keys(classes)) {
        const cls = classes[classId];
        const level = cls.level ?? 0;
        const hitDice = cls.hitDice || 'd6';
        if (level > 0) {
            parts.push(String(level) + hitDice);
        }
    }
    return parts.join(', ');
}

export function getClassSummary(character) {
    const classes = character.classes || {};
    const parts = [];
    for (const classId of Object.keys(classes)) {
        const cls = classes[classId];
        const level = cls.level ?? 0;
        if (level > 0) {
            parts.push(formatClassName(classId) + ' ' + String(level));
        }
    }
    return parts.join(' / ');
}

export function getTotalLevel(character) {
    const classes = character.classes || {};
    let total = 0;
    for (const classId of Object.keys(classes)) {
        total += classes[classId].level ?? 0;
    }
    return total;
}

export function stripSpellNameForLookup(displayName) {
    let s = displayName.trim();
    for (;;) {
        const next = stripOneTrailingBracketContext(s);
        if (next !== null) {
            s = next;
            continue;
        }
        const nextParen = stripOneTrailingParenContext(s);
        if (nextParen !== null) {
            s = nextParen;
            continue;
        }
        break;
    }
    return s;
}

function stripOneTrailingBracketContext(s) {
    const pos = s.lastIndexOf(' [');
    if (pos === -1 || pos + 1 >= s.length || s[pos + 1] !== '[') {
        return null;
    }
    let depth = 0;
    for (let j = pos + 1; j < s.length; j++) {
        if (s[j] === '[') {
            depth++;
        } else if (s[j] === ']') {
            depth--;
            if (depth === 0) {
                return s.slice(0, pos).trimEnd();
            }
        }
    }
    return null;
}

function stripOneTrailingParenContext(s) {
    const pos = s.lastIndexOf(' (');
    if (pos === -1 || pos + 1 >= s.length || s[pos + 1] !== '(') {
        return null;
    }
    let depth = 0;
    for (let j = pos + 1; j < s.length; j++) {
        if (s[j] === '(') {
            depth++;
        } else if (s[j] === ')') {
            depth--;
            if (depth === 0) {
                return s.slice(0, pos).trimEnd();
            }
        }
    }
    return null;
}

export function spellBracketSuffixAsParenForm(displayName) {
    let s = displayName.trim();
    const pos = s.lastIndexOf(' [');
    if (pos === -1 || pos + 1 >= s.length || s[pos + 1] !== '[') {
        return '';
    }
    let depth = 1;
    let j = pos + 2;
    for (; j < s.length; j++) {
        if (s[j] === '[') {
            depth++;
        } else if (s[j] === ']') {
            depth--;
            if (depth === 0) {
                break;
            }
        }
    }
    if (depth !== 0 || j !== s.length - 1) {
        return '';
    }
    const base = s.slice(0, pos).trimEnd();
    const inner = s.slice(pos + 2, j);
    return base + ' (' + inner + ')';
}

export function collectUniqueSpells(character) {
    const seen = new Set();
    const result = [];
    const classes = character.classes || {};
    for (const classId of Object.keys(classes)) {
        const spells = classes[classId].spells;
        if (!spells) {
            continue;
        }
        for (const levelKey of Object.keys(spells)) {
            if (levelKey === '0_extra') {
                continue;
            }
            const list = spells[levelKey];
            if (!Array.isArray(list)) {
                continue;
            }
            for (const name of list) {
                const trimmed = name.trim();
                if (!trimmed || trimmed.startsWith('-')) {
                    continue;
                }
                if (!seen.has(trimmed)) {
                    seen.add(trimmed);
                    result.push({ display: trimmed });
                }
            }
        }
    }
    return result;
}

export function formatWeaponDamage(damage) {
    if (!damage || !damage.base) {
        return '';
    }
    const base = damage.base;
    let text = base.dice || '';
    if (base.bonus) {
        text += formatSignedInt(base.bonus);
    }
    if (base.type) {
        text += ' ' + base.type;
    }
    const extra = damage.extra;
    if (Array.isArray(extra) && extra.length > 0) {
        text += ' ' + extra.join(' ');
    }
    return text.trim();
}

export function formatArmorAc(armor) {
    const ac = armor.ac;
    if (!ac) {
        return '';
    }
    if (typeof ac.fixmod === 'number') {
        return 'AC ' + formatSignedInt(ac.fixmod);
    }
    if (typeof ac.base === 'number') {
        let text = 'AC ' + String(ac.base);
        if (ac.modstat) {
            text += ' + ' + capitalizeFirst(ac.modstat);
            if (Object.prototype.hasOwnProperty.call(ac, 'modcap')) {
                text += ' (max ' + String(ac.modcap) + ')';
            }
        }
        return text;
    }
    return '';
}
