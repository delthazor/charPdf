import {
    ABILITIES,
    SKILL_DEFS,
    calcModFromStatVal,
    capitalizeFirst,
    collectUniqueSpells,
    formatArmorAc,
    formatClassName,
    formatSignedInt,
    formatWeaponDamage,
    getAc,
    getHitDiceString,
    getInitiative,
    getPassivePerception,
    getProfBonus,
    getSaveBonus,
    getSkillBonus,
    getSkillValue,
    getSpellAttackBonus,
    getSpellSaveDc,
    getStatMod,
} from './dnd.js';
import { campaignIdFromSlug, lookupSpell, lookupTrait } from './data.js';

export function renderCampaignPicker(app, manifest, siteBase) {
    app.replaceChildren();

    const wrap = el('div', 'landing');
    wrap.appendChild(el('h1', 'landing-title', 'Choose a campaign'));

    const campaigns = manifest && Array.isArray(manifest.campaigns) ? manifest.campaigns : [];
    if (campaigns.length === 0) {
        wrap.appendChild(el('p', 'muted', 'No campaigns available. Run make sync-webrender-cfg first.'));
        app.appendChild(wrap);
        return;
    }

    const grid = el('div', 'picker-grid');
    for (const campaign of campaigns) {
        const card = el('a', 'picker-card');
        card.href = siteBase + 'index.html?campaign=' + encodeURIComponent(campaign.id);
        card.appendChild(el('span', 'picker-name', campaign.label || campaign.id));
        const count = (campaign.characters || []).length;
        const meta = el('span', 'picker-meta');
        meta.textContent = count === 1 ? '1 character' : count + ' characters';
        card.appendChild(meta);
        grid.appendChild(card);
    }
    wrap.appendChild(grid);
    app.appendChild(wrap);
}

export function renderCharacterPicker(app, campaignId, characters, siteBase) {
    app.replaceChildren();

    const wrap = el('div', 'landing');
    wrap.appendChild(el('h1', 'landing-title', 'Choose a character'));

    wrap.appendChild(buildBackLink(siteBase, null));

    if (!characters || characters.length === 0) {
        wrap.appendChild(el('p', 'muted', 'No characters in this campaign.'));
        app.appendChild(wrap);
        return;
    }

    const subtitle = el('p', 'muted', campaignId);
    wrap.appendChild(subtitle);

    const grid = el('div', 'picker-grid');
    for (const entry of characters) {
        const card = el('a', 'picker-card');
        card.href = siteBase + 'c/' + entry.slug + '.html';
        card.appendChild(el('span', 'picker-name', entry.name));
        const meta = el('span', 'picker-meta');
        meta.textContent = [entry.race, entry.background].filter(Boolean).join(' · ');
        card.appendChild(meta);
        if (entry.classSummary) {
            card.appendChild(el('span', 'picker-class', entry.classSummary));
        }
        grid.appendChild(card);
    }
    wrap.appendChild(grid);
    app.appendChild(wrap);
}

export function renderLanding(app, manifest, siteBase, campaignId) {
    if (campaignId) {
        const group = (manifest.campaigns || []).find(function (item) {
            return item.id === campaignId;
        });
        renderCharacterPicker(app, campaignId, group ? group.characters : [], siteBase);
        return;
    }
    renderCampaignPicker(app, manifest, siteBase);
}

export function renderError(app, message, siteBase, campaignId) {
    app.replaceChildren();
    const wrap = el('div', 'state-message error-state');
    wrap.appendChild(el('h1', null, message));
    wrap.appendChild(buildBackLink(siteBase, campaignId));
    app.appendChild(wrap);
}

export function renderLoading(app) {
    app.replaceChildren();
    const wrap = el('div', 'state-message loading-state');
    wrap.appendChild(el('div', 'spinner'));
    wrap.appendChild(el('p', null, 'Loading character…'));
    app.appendChild(wrap);
}

export function renderCharacterSheet(app, bundle, siteBase) {
    app.replaceChildren();

    const campaignId = campaignIdFromSlug(bundle.entry.slug);
    const back = buildBackLink(siteBase, campaignId);
    back.classList.add('sheet-back-link');
    app.appendChild(back);

    const character = bundle.character;
    const header = buildHeader(character);
    app.appendChild(header);

    const tabBar = el('div', 'tab-bar');
    const panels = el('div', 'tab-panels');

    const tabs = [
        { id: 'overview', label: 'Overview', render: function () { return renderOverview(character); } },
        { id: 'classes', label: 'Classes', render: function () { return renderClasses(character); } },
        { id: 'inventory', label: 'Inventory', render: function () { return renderInventory(character); } },
        { id: 'traits', label: 'Traits', render: function () { return renderTraits(character, bundle.traitsByName); } },
        {
            id: 'spells',
            label: 'Spells',
            render: function () {
                return renderSpells(character, bundle.spellsByName, bundle.spellsByLowerName);
            },
        },
    ];

    const panelNodes = [];
    tabs.forEach(function (tab, index) {
        const btn = el('button', 'tab-btn' + (index === 0 ? ' active' : ''));
        btn.type = 'button';
        btn.textContent = tab.label;
        btn.dataset.tab = tab.id;
        tabBar.appendChild(btn);

        const panel = el('div', 'tab-panel' + (index === 0 ? ' active' : ''));
        panel.dataset.panel = tab.id;
        panel.appendChild(tab.render());
        panels.appendChild(panel);
        panelNodes.push({ btn: btn, panel: panel });
    });

    tabBar.addEventListener('click', function (event) {
        const target = event.target;
        if (!(target instanceof HTMLButtonElement) || !target.dataset.tab) {
            return;
        }
        const tabId = target.dataset.tab;
        panelNodes.forEach(function (item) {
            const active = item.btn.dataset.tab === tabId;
            item.btn.classList.toggle('active', active);
            item.panel.classList.toggle('active', active);
        });
    });

    app.appendChild(tabBar);
    app.appendChild(panels);
}

function buildHeader(character) {
    const header = el('header', 'sheet-header');
    header.appendChild(el('h1', 'char-name', character.name || 'Unknown'));

    const chips = el('div', 'chips');
    if (character.race) {
        chips.appendChild(chip(character.race, 'chip-teal'));
    }
    if (character.background) {
        chips.appendChild(chip(character.background, 'chip-sage'));
    }
    header.appendChild(chips);
    return header;
}

function chip(text, className) {
    return el('span', 'chip ' + className, text);
}

function renderOverview(character) {
    const wrap = el('div', 'overview-grid');

    wrap.appendChild(buildStatGrid(character));
    wrap.appendChild(buildCombatBox(character));
    wrap.appendChild(buildProfListsCard(character));
    wrap.appendChild(buildSkillsCard(character));
    wrap.appendChild(buildSavesCard(character));

    return wrap;
}

function buildStatGrid(character) {
    const card = cardSection('Ability Scores', 'accent-teal');
    card.classList.add('overview-full');
    const grid = el('div', 'stat-grid');
    const stats = character.stats || {};

    for (const stat of ABILITIES) {
        const val = stats[stat] ?? 10;
        const mod = calcModFromStatVal(val);
        const box = el('div', 'stat-box');
        box.appendChild(el('span', 'stat-label', capitalizeFirst(stat).slice(0, 3)));
        box.appendChild(el('span', 'stat-mod', formatSignedInt(mod)));
        box.appendChild(el('span', 'stat-val', String(val)));
        grid.appendChild(box);
    }
    card.appendChild(grid);
    return card;
}

function buildCombatBox(character) {
    const card = cardSection('Combat', 'accent-gold');
    const stats = character.stats || {};
    const rows = [
        ['AC', String(getAc(character))],
        ['HP', String(stats.maxHp ?? '—')],
        ['Speed', String(stats.speed ?? '—') + ' ft'],
        ['Initiative', formatSignedInt(getInitiative(character))],
        ['Passive Perception', String(getPassivePerception(character))],
        ['Proficiency', formatSignedInt(getProfBonus(character))],
        ['Hit Dice', getHitDiceString(character) || '—'],
    ];
    card.appendChild(keyValueList(rows));
    return card;
}

function buildSkillsCard(character) {
    const card = cardSection('Skills', 'accent-blue');
    const list = el('div', 'skill-list');
    for (const skill of SKILL_DEFS) {
        const value = getSkillValue(character, skill.stat, skill.key);
        const bonus = getSkillBonus(character, skill.stat, value);
        const row = el('div', 'skill-row');
        row.appendChild(profIndicator(value));
        row.appendChild(el('span', 'skill-name', skill.label));
        row.appendChild(el('span', 'skill-bonus', formatSignedInt(bonus)));
        list.appendChild(row);
    }
    card.appendChild(list);
    return card;
}

function profIndicator(value) {
    const wrap = el('span', 'prof-dot-wrap');
    if (value >= 1) {
        wrap.appendChild(el('span', 'prof-dot filled'));
    }
    if (value >= 2) {
        wrap.appendChild(el('span', 'prof-dot ring'));
    }
    if (value === 0) {
        wrap.appendChild(el('span', 'prof-dot empty'));
    }
    return wrap;
}

function buildSavesCard(character) {
    const card = cardSection('Saving Throws', 'accent-sage');
    const list = el('div', 'skill-list');
    for (const stat of ABILITIES) {
        const saves = character.proficiencies?.savingThrows || {};
        const profValue = typeof saves[stat] === 'number' ? saves[stat] : 0;
        const bonus = getSaveBonus(character, stat);
        const row = el('div', 'skill-row');
        row.appendChild(profIndicator(profValue));
        row.appendChild(el('span', 'skill-name', capitalizeFirst(stat)));
        row.appendChild(el('span', 'skill-bonus', formatSignedInt(bonus)));
        list.appendChild(row);
    }
    card.appendChild(list);
    return card;
}

function buildProfListsCard(character) {
    const card = cardSection('Proficiencies', 'accent-teal');
    const prof = character.proficiencies || {};
    const sections = [
        ['Languages', prof.languages],
        ['Tools', prof.tools],
        ['Armor', prof.armors],
        ['Simple Weapons', prof.simpleWeapons],
        ['Martial Weapons', prof.martialWeapons],
    ];
    for (const [title, items] of sections) {
        if (!Array.isArray(items) || items.length === 0) {
            continue;
        }
        const block = el('div', 'prof-block');
        block.appendChild(el('h3', 'prof-heading', title));
        block.appendChild(el('p', 'prof-items', items.join(', ')));
        card.appendChild(block);
    }
    return card;
}

function renderClasses(character) {
    const wrap = el('div', 'classes-stack');
    const classes = character.classes || {};
    const ids = Object.keys(classes);
    if (ids.length === 0) {
        wrap.appendChild(el('p', 'muted', 'No class data.'));
        return wrap;
    }

    for (const classId of ids) {
        wrap.appendChild(buildClassCard(character, classId, classes[classId]));
    }
    return wrap;
}

function buildClassCard(character, classId, cls) {
    const card = cardSection(formatClassName(classId), 'accent-gold');
    const header = el('div', 'class-header');
    header.appendChild(el('span', 'class-level', 'Level ' + String(cls.level ?? 0)));
    if (cls.subclass) {
        header.appendChild(el('span', 'class-subclass', cls.subclass));
    }
    if (cls.hitDice) {
        header.appendChild(el('span', 'class-meta', 'Hit Die: ' + cls.hitDice));
    }
    if (typeof cls.resourcePoints === 'number' && cls.resourcePoints > 0) {
        header.appendChild(el('span', 'class-meta', 'Resources: ' + String(cls.resourcePoints)));
    }
    card.appendChild(header);

    const castStat = cls.castStat || '';
    if (castStat) {
        const castBlock = el('div', 'cast-block');
        const mod = getStatMod(character, castStat);
        castBlock.appendChild(el('p', null, 'Spellcasting: ' + capitalizeFirst(castStat) + ' (' + formatSignedInt(mod) + ')'));
        const dc = getSpellSaveDc(character, castStat);
        const atk = getSpellAttackBonus(character, castStat);
        if (dc !== null) {
            castBlock.appendChild(el('p', null, 'Spell Save DC: ' + String(dc)));
        }
        if (atk !== null) {
            castBlock.appendChild(el('p', null, 'Spell Attack: ' + formatSignedInt(atk)));
        }
        card.appendChild(castBlock);
    }

    if (cls.spellslots) {
        const slots = el('div', 'spell-slots');
        slots.appendChild(el('h3', 'sub-heading', 'Spell Slots'));
        const slotRow = el('div', 'slot-row');
        for (let level = 1; level <= 9; level++) {
            const key = String(level);
            const count = cls.spellslots[key] ?? 0;
            if (count > 0) {
                const slot = el('span', 'slot-chip', 'L' + key + ': ' + String(count));
                slotRow.appendChild(slot);
            }
        }
        if (slotRow.childElementCount === 0) {
            slotRow.appendChild(el('span', 'muted', 'None'));
        }
        slots.appendChild(slotRow);
        card.appendChild(slots);
    }

    if (cls.spells) {
        const spellBlock = el('div', 'class-spells');
        spellBlock.appendChild(el('h3', 'sub-heading', 'Spells & Features'));

        const levelKeys = Object.keys(cls.spells).sort(function (a, b) {
            const na = a === '0_extra' ? -0.5 : parseFloat(a);
            const nb = b === '0_extra' ? -0.5 : parseFloat(b);
            return na - nb;
        });

        const tabDefs = [];
        for (const levelKey of levelKeys) {
            const list = cls.spells[levelKey];
            if (!Array.isArray(list) || list.length === 0) {
                continue;
            }
            const label = spellLevelLabel(levelKey);
            const ul = el('ul', 'spell-name-list');
            for (const name of list) {
                ul.appendChild(el('li', null, name));
            }
            tabDefs.push({
                id: 'level-' + levelKey.replace(/\s+/g, '-'),
                label: label,
                content: ul,
            });
        }

        if (tabDefs.length > 0) {
            attachSubTabs(spellBlock, tabDefs);
        } else {
            spellBlock.appendChild(el('p', 'muted', 'None'));
        }
        card.appendChild(spellBlock);
    }

    return card;
}

function renderInventory(character) {
    const wrap = el('div', 'inventory-grid');
    wrap.appendChild(buildEquipmentSection('Equipped', character.equipment?.used));
    wrap.appendChild(buildEquipmentSection('Stashed', character.equipment?.stashed));
    wrap.appendChild(buildBackpackSection(character.backpack));
    return wrap;
}

function buildEquipmentSection(title, bucket) {
    const card = cardSection(title, 'accent-blue');
    if (!bucket) {
        card.appendChild(el('p', 'muted', 'None'));
        return card;
    }
    appendWeaponList(card, bucket.weapons, 'Weapons');
    appendArmorList(card, bucket.armors, 'Armor');
    if (card.querySelector('.equip-list') === null) {
        card.appendChild(el('p', 'muted', 'None'));
    }
    return card;
}

function appendWeaponList(card, weapons, heading) {
    if (!Array.isArray(weapons) || weapons.length === 0) {
        return;
    }
    card.appendChild(el('h3', 'sub-heading', heading));
    const list = el('div', 'equip-list');
    for (const weapon of weapons) {
        const item = el('div', 'equip-item');
        item.appendChild(el('strong', null, weapon.name || 'Weapon'));
        const details = [];
        if (weapon.type) {
            details.push(weapon.type);
        }
        const dmg = formatWeaponDamage(weapon.damage);
        if (dmg) {
            details.push(dmg);
        }
        if (Array.isArray(weapon.props) && weapon.props.length > 0) {
            details.push(weapon.props.join(', '));
        }
        if (weapon.range) {
            details.push('Range ' + weapon.range);
        }
        if (weapon.extratext) {
            details.push(weapon.extratext);
        }
        if (details.length > 0) {
            item.appendChild(el('p', 'equip-detail', details.join(' · ')));
        }
        list.appendChild(item);
    }
    card.appendChild(list);
}

function appendArmorList(card, armors, heading) {
    if (!Array.isArray(armors) || armors.length === 0) {
        return;
    }
    card.appendChild(el('h3', 'sub-heading', heading));
    const list = el('div', 'equip-list');
    for (const armor of armors) {
        const item = el('div', 'equip-item');
        item.appendChild(el('strong', null, armor.name || 'Armor'));
        const details = [];
        if (armor.type) {
            details.push(armor.type);
        }
        const acText = formatArmorAc(armor);
        if (acText) {
            details.push(acText);
        }
        if (armor.extratext) {
            details.push(armor.extratext);
        }
        if (details.length > 0) {
            item.appendChild(el('p', 'equip-detail', details.join(' · ')));
        }
        list.appendChild(item);
    }
    card.appendChild(list);
}

function buildBackpackSection(backpack) {
    const card = cardSection('Backpack', 'accent-sage');
    card.classList.add('inventory-backpack');
    const columns = el('div', 'backpack-columns');
    const sections = [
        ['Accessories', 'accessories'],
        ['Consumables', 'consumables'],
        ['Kits & Tools', 'kits & tools'],
        ['General', 'general'],
    ];

    for (const [label, key] of sections) {
        const items = backpack && Array.isArray(backpack[key]) ? backpack[key] : [];
        const block = el('div', 'backpack-column');
        block.appendChild(el('h3', 'sub-heading', label));
        if (items.length === 0) {
            block.appendChild(el('p', 'backpack-empty', '—'));
        } else {
            const ul = el('ul', 'backpack-list');
            for (const item of items) {
                ul.appendChild(el('li', null, item));
            }
            block.appendChild(ul);
        }
        columns.appendChild(block);
    }

    card.appendChild(columns);
    return card;
}

function renderTraits(character, traitsByName) {
    const wrap = el('div', 'reference-stack');
    const traits = character.traits || [];
    if (traits.length === 0) {
        wrap.appendChild(el('p', 'muted', 'No traits.'));
        return wrap;
    }
    for (const name of traits) {
        const entry = lookupTrait(traitsByName, name);
        wrap.appendChild(buildReferenceCard(name, entry));
    }
    return wrap;
}

function renderSpells(character, spellsByName, spellsByLowerName) {
    const wrap = el('div', 'reference-stack');
    const spells = collectUniqueSpells(character);
    if (spells.length === 0) {
        wrap.appendChild(el('p', 'muted', 'No spells.'));
        return wrap;
    }
    for (const spell of spells) {
        const entry = lookupSpell(spellsByName, spellsByLowerName, spell.display);
        wrap.appendChild(buildReferenceCard(spell.display, entry));
    }
    return wrap;
}

function buildReferenceCard(title, entry) {
    const card = el('article', 'reference-card');
    const titleEl = el('h3', 'reference-title');
    titleEl.appendChild(document.createTextNode(title));
    if (entry && entry.concentration === true) {
        titleEl.appendChild(el('span', 'concentration-tag', '(Concentration)'));
    }
    card.appendChild(titleEl);
    if (!entry) {
        card.appendChild(el('p', 'missing-text', 'Description not found in catalog.'));
        return card;
    }
    if (entry.params) {
        card.appendChild(el('p', 'reference-params', entry.params));
    }
    if (entry.description) {
        const desc = el('div', 'reference-body');
        desc.textContent = entry.description.trim();
        card.appendChild(desc);
    }
    if (entry.upgrades) {
        card.appendChild(el('p', 'reference-upgrades', entry.upgrades));
    }
    return card;
}

function spellLevelLabel(levelKey) {
    if (levelKey === '0') {
        return 'Cantrips';
    }
    if (levelKey === '0_extra') {
        return 'Features';
    }
    return 'Level ' + levelKey;
}

function attachSubTabs(parent, tabDefs) {
    const bar = el('div', 'sub-tab-bar');
    const panels = el('div', 'sub-tab-panels');
    const panelNodes = [];

    tabDefs.forEach(function (tab, index) {
        const btn = el('button', 'sub-tab-btn' + (index === 0 ? ' active' : ''));
        btn.type = 'button';
        btn.textContent = tab.label;
        btn.dataset.subTab = tab.id;
        bar.appendChild(btn);

        const panel = el('div', 'sub-tab-panel' + (index === 0 ? ' active' : ''));
        panel.dataset.subPanel = tab.id;
        panel.appendChild(tab.content);
        panels.appendChild(panel);
        panelNodes.push({ btn: btn, panel: panel, id: tab.id });
    });

    bar.addEventListener('click', function (event) {
        const target = event.target;
        if (!(target instanceof HTMLButtonElement) || !target.dataset.subTab) {
            return;
        }
        const tabId = target.dataset.subTab;
        panelNodes.forEach(function (item) {
            const active = item.id === tabId;
            item.btn.classList.toggle('active', active);
            item.panel.classList.toggle('active', active);
        });
    });

    parent.appendChild(bar);
    parent.appendChild(panels);
}

function cardSection(title, accentClass) {
    const card = el('section', 'card ' + accentClass);
    card.appendChild(el('h2', 'card-title', title));
    return card;
}

function keyValueList(rows) {
    const list = el('dl', 'kv-list');
    for (const [key, value] of rows) {
        list.appendChild(el('dt', null, key));
        list.appendChild(el('dd', null, value));
    }
    return list;
}

function buildBackLink(siteBase, campaignId) {
    const link = el('a', 'back-link');
    if (campaignId) {
        link.href = siteBase + 'index.html?campaign=' + encodeURIComponent(campaignId);
        link.textContent = 'Back to character list';
    } else {
        link.href = siteBase + 'index.html';
        link.textContent = 'Back to campaigns';
    }
    return link;
}

function el(tag, className, text) {
    const node = document.createElement(tag);
    if (className) {
        node.className = className;
    }
    if (text !== undefined) {
        node.textContent = text;
    }
    return node;
}

export function updateDocumentTitle(character) {
    const name = character.name || 'Character';
    document.title = name + ' — Character Sheet';
    const desc = document.querySelector('meta[name="description"]');
    if (desc) {
        const parts = [character.race, character.background].filter(Boolean);
        desc.setAttribute('content', parts.join(' ') + ' — D&D 5e character sheet');
    }
}
