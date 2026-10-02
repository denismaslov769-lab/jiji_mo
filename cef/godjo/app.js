/* Godjo Role Play — CEF интерфейс (vanilla JS, без сборки) */
(function () {
  'use strict';
  const DEMO = typeof window.cef === 'undefined';
  if (DEMO) {
    // мок для отладки в обычном браузере
    const handlers = {};
    window.cef = {
      on: (n, cb) => { (handlers[n] = handlers[n] || []).push(cb); },
      emit: (n, ...a) => console.log('[emit]', n, ...a),
      set_focus: () => {}, hide: () => {},
      _fire: (n, ...a) => (handlers[n] || []).forEach(cb => cb(...a))
    };
    document.body.classList.add('demo');
  }

  // ---------- утилиты ----------
  const $ = (s, r = document) => r.querySelector(s);
  const $$ = (s, r = document) => Array.from(r.querySelectorAll(s));
  const h = (html) => { const t = document.createElement('template'); t.innerHTML = html.trim(); return t.content.firstElementChild; };
  const esc = (s) => String(s == null ? '' : s).replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
  const strip = (s) => String(s).replace(/\{[0-9A-Fa-f]{6}\}/g, '');
  const money = (n) => '$' + Number(n || 0).toLocaleString('ru-RU').replace(/\u00a0/g, ' ');
  const enc = (v) => String(v == null ? '' : v).replace(/%/g, '%25').replace(/ /g, '%20').replace(/\|/g, '%7C');
  function send(...f) { cef.emit('gj', f.map(enc).join('|')); }
  const parse = (j) => { try { return typeof j === 'string' ? JSON.parse(j) : (j || {}); } catch (e) { console.warn('bad json', j); return {}; } };

  const S = {};             // экраны
  let cur = null;           // текущий экран
  const layer = $('#layer');
  let ITEMS = {};           // каталог предметов
  let HUD = {};
  let chatOpen = false;

  function open(name, data) {
    if (!S[name]) return console.warn('нет экрана', name);
    closeCtx();
    if (cur && cur !== name && S[cur].close) S[cur].close(true);
    cur = name;
    layer.innerHTML = '';
    layer.classList.toggle("on", !S[name].noDim);
    document.body.classList.add("modal");
    S[name].open(data);
  }
  function close(name, fromServer) {
    if (name && cur !== name) return;
    if (cur && S[cur].close) S[cur].close();
    cur = null; layer.innerHTML = ''; layer.classList.remove('on'); document.body.classList.remove('modal'); closeCtx();
    if (!fromServer) send('sys', 'closed');
  }
  function win(cls, title, sub, body, foot) {
    const w = h(`<div class="win ${cls}"><div class="win-h"><h2>${title}${sub ? `<span class="sub">${sub}</span>` : ''}</h2><button class="x" data-x>×</button></div><div class="win-b"></div>${foot ? '<div class="win-f"></div>' : ''}</div>`);
    if (typeof body === 'string') $('.win-b', w).innerHTML = body; else if (body) $('.win-b', w).append(body);
    $('[data-x]', w).onclick = () => close();
    layer.append(w);
    return w;
  }

  // ---------- уведомления ----------
  function notify(type, text) {
    const n = h(`<div class="nt ${esc(type)}">${esc(strip(text))}</div>`);
    $('#notify').append(n);
    while ($('#notify').children.length > 5) $('#notify').firstChild.remove();
    setTimeout(() => { n.classList.add('out'); setTimeout(() => n.remove(), 350); }, 4200 + Math.min(4000, text.length * 30));
  }

  // ---------- HUD ----------
  const hud = $('#hud');
  hud.innerHTML = `
    <div class="h-top"><span id="h-time">00:00</span><span>ID <b id="h-id">0</b></span><span>Онлайн <b id="h-on">0</b></span></div>
    <div class="h-money"><div class="cash" id="h-cash">$0</div><div class="hb">Банк: <span id="h-bank">$0</span></div></div>
    <div class="h-lvl"><b id="h-lvl">Ур. 1</b><div class="bar"><i id="h-exp" style="width:0"></i></div></div>
    <div class="h-needs">
      <div class="need" id="n-hp" style="--c:#ff5a5a">❤<div class="bar"><i></i></div></div>
      <div class="need" id="n-ar" style="--c:#5aa9ff">🛡<div class="bar"><i></i></div></div>
      <div class="need" id="n-hu" style="--c:#ff9f43">🍞<div class="bar"><i></i></div></div>
      <div class="need" id="n-fa" style="--c:#b98cff">⚡<div class="bar"><i></i></div></div>
    </div>
    <div class="h-wanted" id="h-wanted"></div>`;
  const speedo = h(`<div class="speedo hidden"><div class="v" id="sp-v">0</div><div class="u">КМ/Ч</div><div class="f"><div class="bar"><i id="sp-f"></i></div><small id="sp-ft">100 л</small></div></div>`);
  const det = h(`<div class="det hidden"><div class="row" style="justify-content:space-between;margin-bottom:6px"><b>📡 Металлоискатель</b><span id="det-v">0%</span></div><div class="bar"><i id="det-b"></i></div></div>`);
  document.body.append(speedo, det);
  function setNeed(id, v) { const e = $('#' + id); $('i', e).style.width = Math.max(0, Math.min(100, v)) + '%'; e.classList.toggle('low', v < 15 && id !== 'n-ar'); }
  function onHud(j) {
    const d = HUD = parse(j);
    hud.classList.remove('hidden');
    $('#h-time').textContent = d.time || '';
    $('#h-id').textContent = d.id; $('#h-on').textContent = d.online;
    $('#h-cash').textContent = money(d.cash); $('#h-bank').textContent = money(d.bank);
    $('#h-lvl').textContent = 'Ур. ' + d.lvl; $('#h-exp').style.width = Math.min(100, (d.exp / Math.max(1, d.need)) * 100) + '%';
    setNeed('n-hp', d.hp); setNeed('n-ar', d.ar); setNeed('n-hu', d.hunger); setNeed('n-fa', d.fatigue);
    $('#n-ar').classList.toggle('hidden', !d.ar);
    let st = ''; for (let i = 1; i <= 6; i++) st += `<span class="${i <= d.wanted ? 'on' : ''}">★</span>`;
    $('#h-wanted').innerHTML = d.wanted > 0 ? st : '';
    speedo.classList.toggle('hidden', d.fuel < 0);
    if (d.fuel >= 0) {
      $('#sp-v').textContent = d.speed; $('#sp-f').style.width = d.fuel + '%';
      $('#sp-f').style.setProperty('--c', d.fuel < 15 ? '#ff5a5a' : '#FFC94D'); $('#sp-ft').textContent = d.fuel + ' л';
    }
    det.classList.toggle('hidden', !(d.det > 0));
    if (d.det > 0) { $('#det-v').textContent = d.det + '%'; $('#det-b').style.width = d.det + '%'; }
    if (cur === 'casino' && S.casino.cash) S.casino.cash(d.cash);
  }

  // ---------- квест ----------
  function onQuest(d) {
    const q = $('#quest');
    if (!d.active) { q.classList.add('hidden'); return; }
    q.classList.remove('hidden');
    q.innerHTML = `<div class="qn">${esc(d.quest)} · Глава ${d.chapter}: ${esc(d.chapterName)}</div><div class="qt">${esc(d.title)}</div><div class="qh">${esc(d.hint)}</div>` +
      (d.target > 1 ? `<div class="qp"><div class="bar"><i style="width:${Math.min(100, d.progress / d.target * 100)}%"></i></div><span>${d.progress}/${d.target}</span></div>` : '');
  }
  function onChapter(d) {
    const c = h(`<div class="chapter"><small>Глава ${d.done} завершена</small><div>Глава ${d.next}. ${esc(d.title)}</div></div>`);
    document.body.append(c); setTimeout(() => c.remove(), 4000);
  }
  function onAch(d) {
    const a = h(`<div class="ach"><small>🏆 Достижение</small><div>${esc(d.title)}</div></div>`);
    document.body.append(a); setTimeout(() => a.remove(), 3800);
  }

  // ---------- виджеты (очередь, прогресс, ранение) ----------
  function widget(id, html, cls = '') {
    let w = document.getElementById(id);
    if (!html) { if (w) w.remove(); return; }
    if (!w) { w = h(`<div class="wg ${cls}" id="${id}"></div>`); $('#widgets').append(w); }
    w.className = 'wg ' + cls; w.innerHTML = html;
  }
  let progT = null;
  function onProgress(d) {
    let p = $('.progress'); if (p) p.remove(); clearInterval(progT);
    p = h(`<div class="progress"><div>${esc(d.text)}</div><div class="bar"><i style="width:0"></i></div></div>`);
    document.body.append(p);
    const t0 = Date.now(), ms = d.ms || 3000;
    progT = setInterval(() => {
      const k = Math.min(1, (Date.now() - t0) / ms); $('i', p).style.width = k * 100 + '%';
      if (k >= 1) { clearInterval(progT); setTimeout(() => p.remove(), 300); }
    }, 50);
  }
  let woundT = null;
  function onWounded(d) {
    let w = $('.wounded'); if (w) w.remove(); clearInterval(woundT);
    if (!d.time) return;
    let left = d.time;
    w = h(`<div class="wounded"><div><b>Вы тяжело ранены</b><span class="mut">Вызовите скорую: /call911 · Автоэвакуация через <span id="w-t"></span></span></div></div>`);
    document.body.append(w);
    const tick = () => { $('#w-t', w).textContent = Math.floor(left / 60) + ':' + String(left % 60).padStart(2, '0'); if (--left < 0) { clearInterval(woundT); w.remove(); } };
    tick(); woundT = setInterval(tick, 1000);
  }

  // ======================= ЭКРАНЫ =======================
  // ---------- авторизация ----------
  S.auth = {
    open(d) {
      this.d = d;
      const reg = !d.registered;
      const w = h(`<div class="win auth"><div class="logo">GODJO <b>ROLE PLAY</b></div><div class="tag">Начни с самого дна — поднимись на вершину</div>
        <div class="who">${reg ? 'Регистрация персонажа' : 'С возвращением'}, <b>${esc(d.name)}</b></div>
        <div class="col">${reg ? `
          <input type="password" id="a-p" placeholder="Пароль (6–32 символа)" maxlength="32">
          <input type="password" id="a-p2" placeholder="Повторите пароль" maxlength="32">
          <input type="email" id="a-e" placeholder="E-mail (для восстановления)" maxlength="60">
          <input id="a-r" placeholder="Кто пригласил? Ник (необязательно)" maxlength="24">` : `
          <input type="password" id="a-p" placeholder="Пароль" maxlength="32">`}
          <div class="err" id="a-err"></div>
          <button class="btn gold wide" id="a-go">${reg ? 'Создать аккаунт' : 'Войти'}</button>
        </div><div class="foot"><span>${esc(d.server || 'Godjo Role Play')}</span><span>Онлайн: ${d.online || 0}</span></div></div>`);
      layer.append(w);
      const go = () => {
        $('#a-err').textContent = '';
        const p = $('#a-p').value;
        if (reg) {
          const p2 = $('#a-p2').value, e = $('#a-e').value.trim(), r = $('#a-r').value.trim();
          if (p.length < 6) return this.err({ code: 'pass_len' });
          if (p !== p2) return this.err({ code: 'pass_match' });
          if (!/^[^@\s]+@[^@\s]+\.[^@\s]+$/.test(e)) return this.err({ code: 'email' });
          send('auth', 'register', p, p2, e, r);
        } else { if (!p) return; send('auth', 'login', p); }
        $('#a-go').disabled = true; setTimeout(() => { const b = $('#a-go'); if (b) b.disabled = false; }, 1500);
      };
      $('#a-go').onclick = go;
      $$('input', w).forEach(i => i.onkeydown = (e) => { if (e.key === 'Enter') go(); });
      setTimeout(() => $('#a-p').focus(), 50);
    },
    err(d) {
      const m = { wrong: `Неверный пароль. Осталось попыток: ${d.left}`, banned: 'Слишком много попыток. Доступ временно заблокирован.', pass_len: 'Пароль должен быть от 6 до 32 символов.',
        pass_match: 'Пароли не совпадают.', email: 'Укажите корректный e-mail.', ref: 'Игрок-пригласивший не найден.', db: 'Ошибка базы данных. Попробуйте позже.' };
      const e = $('#a-err'); if (e) e.textContent = m[d.code] || 'Ошибка';
      const b = $('#a-go'); if (b) b.disabled = false;
    },
    noClose: true
  };

  // ---------- создание персонажа ----------
  const ORIGINS = [['🏙', 'Местный', 'Вырос в Лос-Сантосе, знает улицы'], ['🌾', 'Из деревни', 'Приехал за мечтой, крепкие руки'], ['⛓', 'Бывший заключённый', 'Только вышел, начинает заново'], ['💼', 'Бывший бизнесмен', 'Прогорел, но голова на месте']];
  S.char = {
    noDim: true, noClose: true,
    open(d) {
      this.d = d; this.sex = 0; this.idx = 0; this.origin = 0;
      const w = h(`<div class="win char"><div class="win-h"><h2>Новая жизнь<span class="sub">${esc(d.name)} · все начинают с самого дна</span></h2></div>
        <div class="win-b col">
          <div class="lbl">Пол</div><div class="seg" id="c-sex"><button class="on" data-s="0">Мужской</button><button data-s="1">Женский</button></div>
          <div class="lbl">Внешность</div><div class="cards" id="c-skins"></div>
          <div class="note" id="c-note"></div>
          <div class="lbl">Возраст: <b class="gold-t" id="c-agev">25</b></div><input type="range" id="c-age" min="18" max="70" value="25">
          <div class="lbl">Прошлое</div><div class="cards two" id="c-or"></div>
          <div class="err" id="c-err"></div>
          <button class="btn gold wide" id="c-go">Начать игру</button>
        </div></div>`);
      layer.append(w);
      $('#c-or').innerHTML = ORIGINS.map((o, i) => `<div class="card ${i ? '' : 'on'}" data-o="${i}"><span class="big">${o[0]}</span><b>${o[1]}</b><br><small class="mut">${o[2]}</small></div>`).join('');
      $$('#c-sex button').forEach(b => b.onclick = () => { this.sex = +b.dataset.s; this.idx = 0; $$('#c-sex button').forEach(x => x.classList.toggle('on', x === b)); this.skins(); this.pick(); });
      $$('#c-or .card').forEach(c => c.onclick = () => { this.origin = +c.dataset.o; $$('#c-or .card').forEach(x => x.classList.toggle('on', x === c)); });
      $('#c-age').oninput = (e) => $('#c-agev').textContent = e.target.value;
      $('#c-go').onclick = () => send('char', 'done', this.sex, this.idx, $('#c-age').value, this.origin);
      this.skins();
    },
    skins() {
      const list = this.sex ? this.d.f : this.d.m;
      const names = this.sex ? ['Бездомная', 'Старушка', 'Дама в беде'] : ['Бродяга', 'Работяга', 'Старик'];
      $('#c-skins').innerHTML = list.map((s, i) => `<div class="card ${i === this.idx ? 'on' : ''}" data-i="${i}"><span class="big">${this.sex ? '👩' : '🧔'}</span>${names[i]}<br><small class="mut">#${s}</small></div>`).join('');
      $$('#c-skins .card').forEach(c => c.onclick = () => { this.idx = +c.dataset.i; $$('#c-skins .card').forEach(x => x.classList.toggle('on', x === c)); this.pick(); });
      $('#c-note').textContent = this.sex ? 'В GTA San Andreas только один женский скин бездомной (#77), поэтому два других — максимально скромные городские образы.' : 'Вращайте камеру — персонаж показан в игре справа.';
    },
    pick() { send('char', 'skin', this.sex, this.idx); }
  };

  // ---------- меню ----------
  const ICONS = { cash: '💵', fish: '🐟', detector: '📡', shovel: '⛏', car: '🚗', key: '⌨', quest: '📜', chat: '💬', doc: '🪪', home: '🏠', org: '🏛', family: '👪', info: 'ℹ', gps: '📍', food: '🍔', drink: '🥤', phone: '📱', shirt: '👕', hat: '🧢', job: '🧰', med: '💊', gun: '🔫', bank: '🏦', star: '⭐', tune: '🔧', fuel: '⛽', bag: '🎒', ticket: '🎫', heart: '❤' };
  S.menu = {
    open(d) {
      const list = h(`<div class="mlist"></div>`);
      (d.items || []).forEach(it => {
        const b = h(`<button class="mi"><div class="ic">${ICONS[it.i] || ITEM_ICON(it.i) || '•'}</div><div class="tx"><b>${esc(strip(it.t))}</b>${it.d ? `<small>${esc(strip(it.d))}</small>` : ''}</div>${it.p >= 0 ? `<div class="pr">${it.p ? money(it.p) : 'Бесплатно'}</div>` : ''}</button>`);
        b.onclick = () => send('menu', d.menu, it.id, '');
        list.append(b);
      });
      win('menu', esc(strip(d.title)), esc(strip(d.sub || '')), list);
    }
  };
  S.prompt = {
    open(d) {
      const b = h(`<div class="col"><p>${esc(strip(d.text))}</p>${d.input ? `<input id="p-in" placeholder="${esc(d.ph)}" maxlength="100">` : ''}</div>`);
      const w = win('prompt', esc(d.title), '', b, true);
      const ok = h(`<button class="btn gold">${esc(d.ok || 'Ок')}</button>`), no = h(`<button class="btn">Отмена</button>`);
      $('.win-f', w).append(no, ok);
      ok.onclick = () => send('prompt', d.id, 1, d.input ? $('#p-in').value : '');
      no.onclick = () => { send('prompt', d.id, 0, ''); close(); };
      if (d.input) { const i = $('#p-in'); setTimeout(() => i.focus(), 50); i.onkeydown = (e) => { if (e.key === 'Enter') ok.onclick(); }; }
    }
  };
  // универсальный запрос количества (локально)
  function askAmount(title, max, cb) {
    const box = h(`<div class="ctx" style="left:50%;top:45%;transform:translate(-50%,-50%);padding:14px;min-width:260px"><div class="h">${esc(title)}</div><div class="col" style="margin-top:8px"><input type="number" min="1" max="${max}" value="${max}"><div class="row"><button class="btn grow" data-n>Отмена</button><button class="btn gold grow" data-y>Ок</button></div></div></div>`);
    document.body.append(box); ctxEl = box;
    const i = $('input', box); i.focus(); i.select();
    const done = () => { const v = Math.max(1, Math.min(max, parseInt(i.value) || 0)); closeCtx(); cb(v); };
    $('[data-y]', box).onclick = done; $('[data-n]', box).onclick = closeCtx;
    i.onkeydown = (e) => { if (e.key === 'Enter') done(); };
  }

  // ---------- диалог NPC ----------
  let typeT = null;
  S.dialog = {
    noDim: true,
    open(d) {
      const w = h(`<div class="dlg"><div class="who"><div class="ava">👴</div><div><b>${esc(d.npc)}</b><div class="mut" style="font-size:12px">нажмите на ответ или 1–4</div></div></div><div class="text"></div><div class="opts"></div></div>`);
      layer.append(w);
      const t = $('.text', w), txt = strip(d.text); let i = 0; clearInterval(typeT);
      const opts = $('.opts', w);
      const show = () => { opts.innerHTML = ''; (d.options || []).forEach((o, k) => { const b = h(`<button class="opt"><i>${k + 1}</i>${esc(o.t)}</button>`); b.onclick = () => send('dlg', S.dialog.node(d), o.id); opts.append(b); }); };
      typeT = setInterval(() => { i += 2; t.textContent = txt.slice(0, i); if (i >= txt.length) { clearInterval(typeT); show(); } }, 16);
      w.onclick = (e) => { if (i < txt.length && !e.target.closest('.opt')) { i = txt.length; t.textContent = txt; clearInterval(typeT); show(); } };
      this.d = d;
    },
    node(d) { return d.node != null ? d.node : (this.d && this.d.node); },
    key(k) { const b = $$('.dlg .opt')[k - 1]; if (b) b.click(); }
  };

  // ---------- инвентарь ----------
  const KEYICON = { bottle: '🍾', shawarma: '🌯', burger: '🍔', water: '💧', chips: '🥔', phone: '📱', energy: '🥫', rod: '🎣', bait: '🪱', fish_s: '🐟', fish_m: '🐠', fish_b: '🦈', detector: '📡', shovel: '⛏', coin: '🪙', ring: '💍', junk: '🔩', medkit: '🩹', repair: '🔧', canister: '⛽', ore: '🪨', colt: '🔫', deagle: '🔫', shotgun: '🔫', m4: '🔫', ak47: '🔫', baton: '🏏', ammo: '🧨', armour: '🦺', letter: '✉', cigs: '🚬', lockpick: '🗝' };
  function ITEM_ICON(key) { return KEYICON[key] || ''; }
  const itemIcon = (id) => { const it = ITEMS[id]; return it ? (KEYICON[it.k] || '📦') : '❔'; };
  const itemName = (id) => (ITEMS[id] ? ITEMS[id].n : 'Предмет #' + id);
  let ctxEl = null;
  function closeCtx() { if (ctxEl) { ctxEl.remove(); ctxEl = null; } }
  function ctxMenu(x, y, title, actions) {
    closeCtx();
    const c = h(`<div class="ctx" style="left:${x}px;top:${y}px"><div class="h">${esc(title)}</div></div>`);
    actions.forEach(([t, fn]) => { const b = h(`<button>${t}</button>`); b.onclick = () => { closeCtx(); fn(); }; c.append(b); });
    document.body.append(c); ctxEl = c;
    const r = c.getBoundingClientRect(); if (r.bottom > innerHeight) c.style.top = (innerHeight - r.height - 10) + 'px'; if (r.right > innerWidth) c.style.left = (innerWidth - r.width - 10) + 'px';
  }
  function slotsGrid(n, slots, onClick, onMove) {
    const g = h(`<div class="grid"></div>`), map = {};
    (slots || []).forEach(s => map[s[0]] = s);
    for (let i = 0; i < n; i++) {
      const s = map[i];
      const el = h(`<div class="slot" data-i="${i}">${s ? `<span class="t">${esc(itemName(s[1]))}</span>${itemIcon(s[1])}<span class="n">${s[2] > 1 ? s[2] : ''}</span>` : ''}</div>`);
      if (s) {
        el.draggable = !!onMove;
        el.onclick = (e) => onClick(s, e);
        el.oncontextmenu = (e) => { e.preventDefault(); onClick(s, e); };
        el.ondragstart = (e) => { e.dataTransfer.setData('text', i); el.classList.add('drag'); };
        el.ondragend = () => el.classList.remove('drag');
      }
      if (onMove) {
        el.ondragover = (e) => { e.preventDefault(); el.classList.add('over'); };
        el.ondragleave = () => el.classList.remove('over');
        el.ondrop = (e) => { e.preventDefault(); el.classList.remove('over'); const from = +e.dataTransfer.getData('text'); if (from !== i) onMove(from, i); };
      }
      g.append(el);
    }
    return g;
  }
  S.inventory = {
    open(d) { this.w = win('inv', 'Инвентарь', 'ПКМ/клик — действия, перетаскивание — перемещение', ''); this.render(d); },
    render(d) {
      if (!this.w) return;
      const b = $('.win-b', this.w); b.innerHTML = '';
      const kg = (x) => (x / 1000).toFixed(1);
      b.append(h(`<div class="wbar"><span>Вес: <b>${kg(d.w)} / ${kg(d.wm)} кг</b></span><div class="bar"><i style="width:${Math.min(100, d.w / d.wm * 100)}%"></i></div></div>`));
      b.append(slotsGrid(30, d.slots, (s, e) => {
        const it = ITEMS[s[1]] || {}, acts = [];
        if (it.t !== 6) acts.push(['✋ Использовать', () => send('inv', 'use', s[0])]);
        acts.push(['🤝 Передать рядом', () => askAmount('Сколько передать?', s[2], (n) => send('inv', 'give', s[0], n))]);
        acts.push(['🗑 Выбросить', () => askAmount('Сколько выбросить?', s[2], (n) => send('inv', 'drop', s[0], n))]);
        ctxMenu(e.clientX, e.clientY, `${itemName(s[1])} ×${s[2]}`, acts);
      }, (from, to) => send('inv', 'move', from, to)));
    },
    close() { this.w = null; }
  };
  S.storage = {
    open(d) { this.w = win('stor', esc(d.title || 'Хранилище'), 'Клик по предмету — переложить', ''); this.render(d); },
    render(d) {
      if (!this.w) return;
      if (d.title) $('.win-h h2', this.w).firstChild.textContent = d.title;
      const b = $('.win-b', this.w); b.innerHTML = '';
      const two = h(`<div class="two"><div><div class="lbl" style="margin-bottom:8px">Инвентарь</div></div><div><div class="lbl" style="margin-bottom:8px">Хранилище</div></div></div>`);
      two.children[0].append(slotsGrid(30, d.inv.slots, (s) => askAmount('Положить: ' + itemName(s[1]), s[2], (n) => send('stor', 'put', s[0], n))));
      two.children[1].append(slotsGrid(d.max || 40, d.slots, (s) => askAmount('Забрать: ' + itemName(s[1]), s[2], (n) => send('stor', 'take', s[0], n))));
      b.append(two);
    },
    close(sw) { this.w = null; send('stor', 'close'); }
  };

  // ---------- банк ----------
  S.bank = {
    open(d) {
      this.d = d; this.tab = 'wd';
      const b = h(`<div><div class="bcard"><div class="row" style="justify-content:space-between"><b class="logo">GODJO <b>BANK</b></b><span>${d.atm ? 'Банкомат' : 'Отделение'}</span></div>
        <div class="num">${esc(d.card)}</div><div class="row" style="justify-content:space-between;align-items:flex-end"><div><small class="mut">Владелец</small><div>${esc(d.name)}</div></div><div style="text-align:right"><small class="mut">Баланс</small><div class="bal" id="b-bal">${money(d.bank)}</div></div></div></div>
        <div class="tabs"><button data-t="wd" class="on">Снять</button><button data-t="dep">Внести</button><button data-t="tr">Перевод</button></div>
        <div class="col"><div class="mut">Наличные: <b id="b-cash" class="green-t">${money(d.cash)}</b>${d.atm ? ' · Комиссия банкомата 1%' : ''}</div>
          <input id="b-name" class="hidden" placeholder="Получатель: Имя_Фамилия" maxlength="24">
          <input id="b-amt" type="number" min="1" placeholder="Сумма">
          <div class="quick">${[100, 500, 1000, 5000, 10000].map(v => `<button data-v="${v}">${money(v)}</button>`).join('')}<button data-v="all">Всё</button></div>
          <button class="btn gold wide" id="b-go">Снять</button></div></div>`);
      win('bank', 'Банковские услуги', '', b);
      $$('.tabs button', b).forEach(t => t.onclick = () => { this.tab = t.dataset.t; $$('.tabs button', b).forEach(x => x.classList.toggle('on', x === t)); $('#b-name').classList.toggle('hidden', this.tab !== 'tr'); $('#b-go').textContent = { wd: 'Снять', dep: 'Внести', tr: 'Перевести' }[this.tab]; });
      $$('.quick button', b).forEach(q => q.onclick = () => { $('#b-amt').value = q.dataset.v === 'all' ? (this.tab === 'dep' ? this.d.cash : this.d.bank) : q.dataset.v; });
      $('#b-go').onclick = () => { const a = parseInt($('#b-amt').value) || 0; if (a <= 0) return; if (this.tab === 'tr') send('bank', 'tr', a, $('#b-name').value.trim()); else send('bank', this.tab, a); };
    },
    upd(d) { Object.assign(this.d || {}, d); if ($('#b-bal')) { $('#b-bal').textContent = money(d.bank); $('#b-cash').textContent = money(d.cash); } }
  };

  // ---------- паспорт: анкета ----------
  S.passport_form = {
    open(d) {
      const b = h(`<div class="paper col"><div class="row" style="justify-content:space-between"><b>АНКЕТА ФОРМЫ №1-П</b><span>Талон А-${d.ticket}</span></div>
        <div class="row"><div class="grow"><div class="lbl">ФИО</div><b>${esc(d.name.replace('_', ' '))}</b></div><div><div class="lbl">Возраст</div><b>${d.age}</b></div><div><div class="lbl">Пол</div><b>${d.sex ? 'Ж' : 'М'}</b></div></div>
        <div><div class="lbl">Место рождения</div><select id="f-city"><option value="">— выберите —</option>${['Лос-Сантос', 'Сан-Фиерро', 'Лас-Вентурас', 'Паломино-Крик', 'Монтгомери', 'Диллимор', 'Форт-Карсон', 'Эйнджел-Пайн', 'Другой город'].map(c => `<option>${c}</option>`).join('')}</select></div>
        <div><div class="lbl">Семейное положение</div><select id="f-mar"><option value="">— выберите —</option>${(d.sex ? ['Не замужем', 'Замужем', 'Разведена', 'Вдова'] : ['Холост', 'Женат', 'Разведён', 'Вдовец']).map(c => `<option>${c}</option>`).join('')}</select></div>
        <div><div class="lbl">Подпись заявителя</div><div class="sigbox" id="f-sig">Нажмите, чтобы расписаться</div></div>
        <div class="err" id="f-err"></div></div>`);
      const w = win('form', 'Паспортный стол', 'Заполните анкету для получения паспорта', b, true);
      let signed = false;
      $('#f-sig').onclick = () => { signed = true; const s = $('#f-sig'); s.classList.add('on'); s.textContent = d.name.split('_').map(x => x[0]).join('.') + '. ' + (d.name.split('_')[1] || ''); };
      const go = h(`<button class="btn gold">Подать анкету</button>`); $('.win-f', w).append(go);
      go.onclick = () => { if (!$('#f-city').value || !$('#f-mar').value) return this.err({ code: 'fields' }); if (!signed) return this.err({ code: 'sign' }); send('passport', 'submit', $('#f-city').value, $('#f-mar').value, 1); };
    },
    err(d) { const e = $('#f-err'); if (e) e.textContent = d.code === 'sign' ? 'Поставьте подпись.' : 'Заполните все поля.'; }
  };

  // ---------- документы ----------
  S.document = {
    open(d) {
      const nm = esc(String(d.name || '').replace('_', ' '));
      let body;
      if (d.type === 'passport') body = `<div class="dh"><b>ПАСПОРТ ГРАЖДАНИНА SAN ANDREAS</b><span>${esc(d.number)}</span></div><div class="db"><div class="photo">${d.sex ? '👩' : '🧔'}</div><div class="fields">
          <div class="full"><small>Фамилия, имя</small><b>${nm}</b></div><div><small>Возраст</small><b>${d.age}</b></div><div><small>Пол</small><b>${d.sex ? 'Женский' : 'Мужской'}</b></div>
          <div><small>Место рождения</small><b>${esc(d.city)}</b></div><div><small>Семейное положение</small><b>${esc(d.marital)}</b></div>
          <div><small>Организация</small><b>${esc(d.org)}</b></div><div><small>Дата выдачи</small><b>${esc(d.issued)}</b></div>
          <div class="full"><small>Подпись</small><span class="sign">${nm}</span></div></div></div>`;
      else if (d.type === 'med') body = `<div class="dh"><b>МЕДИЦИНСКАЯ КАРТА</b><span>All Saints General</span></div><div class="db"><div class="photo">⚕</div><div class="fields"><div class="full"><small>Пациент</small><b>${nm}</b></div><div><small>Возраст</small><b>${d.age}</b></div><div><small>Заключение</small><b>${esc(d.status)}</b></div><div class="full"><small>Годен к работе и управлению ТС</small><b>Да</b></div></div></div>`;
      else body = `<div class="dh"><b>ЛИЦЕНЗИИ</b><span>DMV San Andreas</span></div><div class="db"><div class="photo">🚗</div><div class="fields"><div class="full"><small>Владелец</small><b>${nm}</b></div><div><small>Теория ПДД</small><b>${d.theory ? '✔ Сдана' : '✘ Нет'}</b></div><div><small>Права кат. B</small><b>${d.drive ? '✔ Есть' : '✘ Нет'}</b></div></div></div>`;
      const w = h(`<div class="win doc ${d.type}">${body}<div class="win-f"><button class="btn">Закрыть</button></div></div>`);
      $('.btn', w).onclick = () => close();
      layer.append(w);
    }
  };

  // ---------- теория ПДД ----------
  S.theory = {
    open(d) {
      this.d = d; this.i = 0; this.ans = d.questions.map(() => -1); this.left = d.time || 300;
      this.w = win('theory', 'Экзамен по теории ПДД', `Нужно ответить правильно минимум на ${d.need} из ${d.questions.length}`, '', true);
      this.render();
      clearInterval(this.t); this.t = setInterval(() => { this.left--; const e = $('#th-t'); if (e) e.textContent = Math.floor(this.left / 60) + ':' + String(this.left % 60).padStart(2, '0'); if (this.left <= 0) this.submit(); }, 1000);
    },
    render() {
      const d = this.d, q = d.questions[this.i], b = $('.win-b', this.w);
      b.innerHTML = `<div class="dots">${d.questions.map((_, k) => `<i class="${k === this.i ? 'cur' : this.ans[k] >= 0 ? 'on' : ''}"></i>`).join('')}</div>
        <div class="row" style="justify-content:space-between;margin-bottom:10px"><span class="mut">Вопрос ${this.i + 1} из ${d.questions.length}</span><span class="gold-t" id="th-t"></span></div>
        <div class="q">${esc(q.q)}</div><div class="ans">${q.a.map((a, k) => `<button data-k="${k}" class="${this.ans[this.i] === k ? 'on' : ''}">${esc(a)}</button>`).join('')}</div>`;
      $$('.ans button', b).forEach(x => x.onclick = () => { this.ans[this.i] = +x.dataset.k; if (this.i < d.questions.length - 1) { this.i++; } this.render(); });
      const f = $('.win-f', this.w); f.innerHTML = '';
      const prev = h(`<button class="btn" ${this.i ? '' : 'disabled'}>Назад</button>`), done = h(`<button class="btn gold" ${this.ans.includes(-1) ? 'disabled' : ''}>Завершить</button>`);
      prev.onclick = () => { this.i--; this.render(); }; done.onclick = () => this.submit();
      f.append(prev, done);
    },
    submit() { clearInterval(this.t); send('theory', ...this.ans.map(a => Math.max(0, a))); },
    result(r) {
      clearInterval(this.t);
      if (!this.w) return;
      $('.win-b', this.w).innerHTML = `<div style="text-align:center;padding:20px"><div style="font-size:60px">${r.passed ? '🎉' : '😔'}</div><h2 style="margin:10px 0">${r.passed ? 'Экзамен сдан!' : 'Экзамен не сдан'}</h2><div class="mut">Правильных ответов: <b class="${r.passed ? 'green-t' : 'red-t'}">${r.right} из ${r.total}</b></div></div>`;
      $('.win-f', this.w).innerHTML = ''; const c = h(`<button class="btn gold">Закрыть</button>`); c.onclick = () => close(); $('.win-f', this.w).append(c);
    },
    close() { clearInterval(this.t); this.w = null; }
  };

  // ---------- телефон ----------
  S.phone = {
    noDim: true,
    open(d) {
      this.d = d; this.view = 'home'; this.dial = '';
      this.w = h(`<div class="phone"><div class="scr"><div class="sbar"><span id="ph-time">${esc(HUD.time || '')}</span><span>📶 Godjo Mobile</span></div><div class="papp"></div></div></div>`);
      layer.append(this.w); this.render();
      if (this.call) this.callState(this.call);
    },
    go(v) { this.view = v; this.render(); },
    head(t) { const e = h(`<div class="ptitle"><button>‹</button>${t}</div>`); $('button', e).onclick = () => this.go('home'); return e; },
    render() {
      const a = $('.papp', this.w), d = this.d; a.innerHTML = '';
      if (this.view === 'home') {
        a.append(h(`<div style="text-align:center;margin-top:24px"><div style="font-size:46px;font-weight:200">${esc(HUD.time || '')}</div><div class="mut" style="font-size:12px">Ваш номер: ${esc(d.number || '—')}</div></div>`));
        const apps = h(`<div class="apps"></div>`);
        [['📞', 'Телефон', '#2ecc71', 'dial'], ['👥', 'Контакты', '#3498db', 'contacts'], ['💬', 'Сообщения', '#9b59b6', 'sms'], ['🚕', 'Такси', '#f1c40f', 'taxi'], ['🏦', 'Банк', '#e67e22', 'bank'], ['🚑', '911', '#e74c3c', '911']].forEach(([i, n, c, v]) => {
          const e = h(`<div class="app"><i style="--c:${c}">${i}</i>${n}</div>`);
          e.onclick = () => { if (v === 'taxi') { send('phone', 'taxi'); notify('info', 'Вызов такси отправлен диспетчеру.'); } else if (v === '911') send('phone', 'call', '911'); else this.go(v); };
          apps.append(e);
        });
        a.append(apps);
      } else if (this.view === 'dial') {
        a.append(this.head('Телефон'));
        const disp = h(`<div class="dial">${esc(this.dial) || '<span class="mut">номер</span>'}</div>`), kp = h(`<div class="keypad"></div>`);
        ['1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '0', '⌫'].forEach(k => { const b = h(`<button>${k}</button>`); b.onclick = () => { this.dial = k === '⌫' ? this.dial.slice(0, -1) : (this.dial + k).slice(0, 10); disp.innerHTML = esc(this.dial) || '<span class="mut">номер</span>'; }; kp.append(b); });
        const c = h(`<button class="callbtn">📞</button>`); c.onclick = () => { if (this.dial) send('phone', 'call', this.dial.replace(/\D/g, '')); };
        a.append(disp, kp, c);
      } else if (this.view === 'contacts') {
        a.append(this.head('Контакты'));
        const l = h(`<div class="plist"></div>`);
        (d.contacts || []).forEach(c => {
          const e = h(`<div class="pitem"><div class="grow"><b>${esc(c.n)}</b><small>${esc(c.num)}</small></div><button title="Позвонить">📞</button><button title="SMS">💬</button>${/^(5550101|911)$/.test(c.num) ? '' : '<button title="Удалить">🗑</button>'}</div>`);
          const bs = $$('button', e); bs[0].onclick = () => send('phone', 'call', c.num); bs[1].onclick = () => { this.smsTo = c.num; this.go('write'); };
          if (bs[2]) bs[2].onclick = () => { send('phone', 'delc', c.num); d.contacts = d.contacts.filter(x => x !== c); this.render(); };
          l.append(e);
        });
        const add = h(`<div class="col" style="margin-top:12px"><input id="pc-n" placeholder="Имя" maxlength="24"><input id="pc-num" placeholder="Номер" maxlength="10"><button class="btn gold">Добавить контакт</button></div>`);
        $('button', add).onclick = () => { const n = $('#pc-n').value.trim(), num = $('#pc-num').value.replace(/\D/g, ''); if (!n || !num) return; send('phone', 'addc', num, n); d.contacts.push({ n, num }); this.render(); };
        a.append(l, add);
      } else if (this.view === 'sms') {
        a.append(this.head('Сообщения'));
        const nb = h(`<button class="btn gold wide" style="margin-bottom:10px">✏ Новое сообщение</button>`); nb.onclick = () => { this.smsTo = ''; this.go('write'); };
        const l = h(`<div class="col" style="gap:2px"></div>`);
        (d.sms || []).forEach(m => { const out = m.f === d.number; l.append(h(`<div class="sms ${out ? 'out' : 'in'}"><b style="font-size:11px">${out ? '→ ' + esc(m.to) : esc(m.f)}</b><br>${esc(m.t)}<small>${esc(m.d)}</small></div>`)); });
        if (!(d.sms || []).length) l.append(h(`<div class="mut" style="text-align:center;margin-top:30px">Сообщений нет</div>`));
        a.append(nb, l);
      } else if (this.view === 'write') {
        a.append(this.head('Новое SMS'));
        const f = h(`<div class="col"><input id="ps-to" placeholder="Номер" maxlength="10" value="${esc(this.smsTo || '')}"><textarea id="ps-t" rows="5" maxlength="100" placeholder="Текст (до 100 символов, $1)"></textarea><button class="btn gold">Отправить</button></div>`);
        $('button', f).onclick = () => { const to = $('#ps-to').value.replace(/\D/g, ''), t = $('#ps-t').value.trim(); if (!to || !t) return; send('phone', 'sms', to, t); (d.sms = d.sms || []).unshift({ f: d.number, to, t, d: 'сейчас' }); this.go('sms'); };
        a.append(f);
      } else if (this.view === 'bank') {
        a.append(this.head('Мобильный банк'));
        const f = h(`<div class="col"><div class="pitem"><div class="grow"><small>Баланс счёта</small><b style="font-size:20px">${money(HUD.bank != null ? HUD.bank : d.bank)}</b></div></div><div class="mut" style="font-size:12px">Перевод по номеру телефона (комиссия 1%)</div><input id="pb-n" placeholder="Номер получателя" maxlength="10"><input id="pb-a" type="number" placeholder="Сумма"><button class="btn gold">Перевести</button></div>`);
        $('button', f).onclick = () => { const n = $('#pb-n').value.replace(/\D/g, ''), am = parseInt($('#pb-a').value) || 0; if (n && am > 0) send('phone', 'transfer', n, am); };
        a.append(f);
      }
    },
    callState(st) {
      this.call = st;
      let c = this.w && $('.callscr', this.w);
      if (st.state === 'idle' || st.state === 'unavailable' || st.state === 'busy') {
        if (c) c.remove(); this.call = null;
        widget('w-call', '');
        if (st.state !== 'idle') notify('error', st.state === 'busy' ? 'Абонент занят.' : 'Абонент недоступен.');
        return;
      }
      if (!this.w) { // телефон закрыт — показываем виджет
        if (st.state === 'incoming') widget('w-call', `📞 Входящий: <b>${esc(st.num)}</b><br><span class="mut">/pickup — ответить, /hangup — сбросить или P</span>`, 'call');
        else widget('w-call', st.state === 'talking' ? '📞 Идёт разговор · пишите в чат · /hangup' : '📞 Вызов...', 'call');
        return;
      }
      if (!c) { c = h(`<div class="callscr"></div>`); $('.scr', this.w).append(c); }
      const lab = { incoming: 'Входящий вызов', ringing: 'Вызов...', talking: 'Идёт разговор — пишите в чат' }[st.state] || '';
      c.innerHTML = `<div style="font-size:60px">👤</div><div class="nm">${esc(st.num || this.dial || '')}</div><div class="mut">${lab}</div><div class="row" style="gap:30px;margin-top:20px">${st.state === 'incoming' ? '<button class="callbtn" data-a>📞</button>' : ''}<button class="callbtn hang" data-h>📵</button></div>`;
      const ans = $('[data-a]', c); if (ans) ans.onclick = () => send('phone', 'answer');
      $('[data-h]', c).onclick = () => send('phone', 'hang');
      widget('w-call', '');
    },
    close() { this.w = null; send('phone', 'close'); if (this.call) this.callState(this.call); }
  };

  // ---------- меню лидера ----------
  S.leader = {
    open(d) {
      this.d = d;
      const rk = (r) => d.ranks[r - 1] || ('Ранг ' + r);
      const b = h(`<div><div class="stats"><div class="stat"><small>Бюджет</small><b class="green-t">${money(d.bank)}</b></div><div class="stat"><small>Материалы</small><b>${d.mats}</b></div><div class="stat"><small>Сотрудников</small><b>${d.members.length}</b></div></div>
        <div class="row" style="margin-bottom:14px"><input id="l-v" type="number" placeholder="Сумма" style="width:180px"><button class="btn" id="l-dep">Внести в бюджет</button>${d.myrank >= 6 ? '<button class="btn" id="l-wd">Снять из бюджета</button>' : ''}</div>
        <table><thead><tr><th>Сотрудник</th><th>Ранг</th><th>Последний вход</th><th></th></tr></thead><tbody></tbody></table></div>`);
      const tb = $('tbody', b);
      d.members.forEach(m => {
        const can = m.r < d.myrank;
        const tr = h(`<tr><td><span class="dot ${m.on ? 'on' : ''}"></span>${esc(m.n.replace('_', ' '))}</td><td>${can ? `<select>${d.ranks.map((n, i) => i + 1 < d.myrank ? `<option value="${i + 1}" ${i + 1 === m.r ? 'selected' : ''}>${i + 1}. ${esc(n)}</option>` : '').join('')}</select>` : esc(m.r + '. ' + rk(m.r))}</td><td class="mut">${esc(m.last || '—')}</td><td>${can ? '<button class="btn red" style="padding:5px 10px">Уволить</button>' : ''}</td></tr>`);
        if (can) { $('select', tr).onchange = (e) => send('leader', 'rank', m.id, e.target.value); $('button', tr).onclick = () => { if (confirm('Уволить ' + m.n + '?')) { send('leader', 'kick', m.id, 0); tr.remove(); } }; }
        tb.append(tr);
      });
      win('leader', esc(d.org), 'Панель руководителя', b);
      $('#l-dep').onclick = () => { const v = parseInt($('#l-v').value) || 0; if (v > 0) send('leader', 'deposit', 0, v); };
      const wd = $('#l-wd'); if (wd) wd.onclick = () => { const v = parseInt($('#l-v').value) || 0; if (v > 0) send('leader', 'withdraw', 0, v); };
    }
  };

  // ---------- казино ----------
  const SYM = ['🍒', '🍋', '🍑', '🔔', '💎', '7️⃣'];
  const REDS = [1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36];
  S.casino = {
    open(d) {
      this.d = d; this.game = 'roulette'; this.bet = { type: 'red', val: 0 }; this.busy = false;
      this.w = win('casino', 'Казино «Четыре Дракона»', `Ставки от ${money(d.min)} до ${money(d.max)} · играйте ответственно`, '');
      this.render();
    },
    cash(v) { const e = $('#cs-cash'); if (e) e.textContent = money(v); },
    render() {
      const b = $('.win-b', this.w);
      b.innerHTML = `<div class="tabs"><button data-g="roulette" class="${this.game === 'roulette' ? 'on' : ''}">🎡 Рулетка</button><button data-g="slots" class="${this.game === 'slots' ? 'on' : ''}">🎰 Слоты</button></div><div id="cs-body"></div>
        <div class="betrow"><span>Наличные: <b class="green-t" id="cs-cash">${money(HUD.cash != null ? HUD.cash : this.d.cash)}</b></span><span class="grow"></span><input id="cs-amt" type="number" value="${this.amt || this.d.min}" min="${this.d.min}" max="${this.d.max}"><button class="btn gold" id="cs-go">${this.game === 'roulette' ? 'Крутить' : 'Играть'}</button></div><div class="res" id="cs-res"></div>`;
      $$('.tabs button', b).forEach(t => t.onclick = () => { if (this.busy) return; this.game = t.dataset.g; this.render(); });
      const body = $('#cs-body', b);
      if (this.game === 'roulette') {
        body.innerHTML = `<div class="wheel"><div class="ball green" id="cs-ball">0</div><div class="mut" style="max-width:300px">Выберите ставку: число (×36), дюжину (×3) или цвет/чёт/половину (×2).</div></div><div class="table" id="cs-t"></div><div class="outs" id="cs-o"></div>`;
        const t = $('#cs-t', body);
        const z = h(`<button class="g" data-ty="num" data-v="0">0</button>`); t.append(z);
        for (let row = 3; row >= 1; row--) for (let c = 0; c < 12; c++) { const n = c * 3 + row; t.append(h(`<button class="${REDS.includes(n) ? 'r' : 'b'}" data-ty="num" data-v="${n}">${n}</button>`)); }
        const o = $('#cs-o', body);
        [['red', 0, 'Красное'], ['black', 0, 'Чёрное'], ['even', 0, 'Чёт'], ['odd', 0, 'Нечет'], ['low', 0, '1–18'], ['high', 0, '19–36'], ['dozen', 1, '1-я 12'], ['dozen', 2, '2-я 12'], ['dozen', 3, '3-я 12']].forEach(([ty, v, n]) => o.append(h(`<button data-ty="${ty}" data-v="${v}">${n}</button>`)));
        $$('[data-ty]', body).forEach(x => { if (x.dataset.ty === this.bet.type && +x.dataset.v === this.bet.val) x.classList.add('on'); x.onclick = () => { this.bet = { type: x.dataset.ty, val: +x.dataset.v }; $$('[data-ty]', body).forEach(y => y.classList.toggle('on', y === x)); }; });
      } else {
        body.innerHTML = `<div class="reels">${[0, 1, 2].map(() => `<div class="reel">${SYM[5]}</div>`).join('')}</div><div class="mut" style="text-align:center">Три 7️⃣ — ×100 · три 💎 — ×25 · три 🔔 — ×15 · две 🍒 подряд — ×2 · пара — возврат</div>`;
      }
      $('#cs-go', b).onclick = () => {
        if (this.busy) return;
        const a = parseInt($('#cs-amt').value) || 0; this.amt = a;
        if (a < this.d.min || a > this.d.max) return notify('error', `Ставка от ${money(this.d.min)} до ${money(this.d.max)}`);
        this.busy = true; $('#cs-res').textContent = '';
        if (this.game === 'roulette') { $('#cs-ball').className = 'ball spin'; send('casino', 'roulette', this.bet.type, this.bet.val, a); }
        else { $$('.reel').forEach(r => r.classList.add('spin')); this.spinT = setInterval(() => $$('.reel').forEach(r => r.textContent = SYM[Math.random() * 6 | 0]), 80); send('casino', 'slots', a); }
        setTimeout(() => { if (this.busy) { this.busy = false; $$('.reel').forEach(r => r.classList.remove('spin')); clearInterval(this.spinT); const bl = $('#cs-ball'); if (bl) bl.className = 'ball green'; } }, 6000);
      };
    },
    result(r) {
      if (!this.w) return;
      const fin = () => { this.busy = false; this.cash(r.cash); const e = $('#cs-res'); if (e) e.innerHTML = r.win > 0 ? `<span class="green-t">Выигрыш ${money(r.win)}!</span>` : `<span class="red-t">Ставка ${money(r.bet)} проиграна</span>`; };
      if (r.game === 'roulette') {
        setTimeout(() => { const bl = $('#cs-ball'); if (bl) { bl.className = 'ball ' + (r.n === 0 ? 'green' : r.red ? 'red' : 'black'); bl.textContent = r.n; } fin(); }, 1600);
      } else {
        const reels = $$('.reel');
        r.reels.forEach((s, i) => setTimeout(() => { if (i === 2) clearInterval(this.spinT); if (reels[i]) { reels[i].classList.remove('spin'); reels[i].textContent = SYM[s]; } if (i === 2) { reels.forEach((x, k) => x.textContent = SYM[r.reels[k]]); fin(); } }, 700 + i * 450));
      }
    },
    close() { clearInterval(this.spinT); this.w = null; }
  };

  // ---------- рыбалка ----------
  S.fishing = {
    noDim: true,
    open(d) {
      this.d = d; this.hits = 0; this.miss = 0; this.need = 3; this.pos = 0; this.dir = 1; this.left = (d.time || 10) * 10;
      this.w = h(`<div class="fish"><b style="font-size:18px">🎣 Клюёт!</b><div class="mut" style="font-size:12px;margin-top:4px">Жмите <span class="kbd">ПРОБЕЛ</span> или кликайте, когда метка в зелёной зоне. Нужно ${this.need} попадания, ошибок — не больше 2.</div>
        <div class="track"><div class="zone"></div><div class="mk"></div></div><div class="hits"></div><div class="mut" id="fi-t" style="font-size:12px;margin-top:6px"></div></div>`);
      layer.append(this.w);
      this.newZone(); this.draw();
      this.t = setInterval(() => this.step(), 16);
      this.tt = setInterval(() => { this.left--; $('#fi-t').textContent = 'Осталось: ' + (this.left / 10).toFixed(1) + ' с'; if (this.left <= 0) this.finish(false); }, 100);
      this.w.onclick = () => this.hit();
    },
    newZone() { const z = this.d.zone || 20; this.zs = 5 + Math.random() * (90 - z); this.ze = this.zs + z; const e = $('.zone', this.w); e.style.left = this.zs + '%'; e.style.width = z + '%'; },
    step() { const sp = 0.6 + (10 - (this.d.speed || 6)) * 0.12 + this.hits * 0.15; this.pos += this.dir * sp; if (this.pos >= 100) { this.pos = 100; this.dir = -1; } if (this.pos <= 0) { this.pos = 0; this.dir = 1; } $('.mk', this.w).style.left = 'calc(' + this.pos + '% - 3px)'; },
    draw() { $('.hits', this.w).innerHTML = Array.from({ length: this.need }, (_, i) => i < this.hits ? '🐟' : '◌').join('') + ' ' + '❌'.repeat(this.miss); },
    hit() {
      if (!this.w) return;
      if (this.pos >= this.zs && this.pos <= this.ze) { this.hits++; if (this.hits >= this.need) return this.finish(true); this.newZone(); }
      else { this.miss++; if (this.miss > 2) return this.finish(false); }
      this.draw();
    },
    finish(ok) { if (this.done) return; this.done = true; clearInterval(this.t); clearInterval(this.tt); send('fish', 'result', ok ? 1 : 0); },
    open_reset() { this.done = false; },
    close() { clearInterval(this.t); clearInterval(this.tt); if (!this.done) send('fish', 'cancel'); this.done = false; this.w = null; }
  };
  const _fo = S.fishing.open; S.fishing.open = function (d) { this.done = false; _fo.call(this, d); };

  // ======================= события от сервера =======================
  cef.on('gj:open', (name, json) => open(name, parse(json)));
  cef.on('gj:close', (name) => close(name || null, true));
  cef.on('gj:notify', (type, text) => notify(type, text));
  cef.on('gj:hud', (json) => onHud(json));
  cef.on('gj:data', (key, json) => {
    const d = parse(json);
    switch (key) {
      case 'items': ITEMS = {}; (Array.isArray(d) ? d : []).forEach(i => ITEMS[i.id] = i); break;
      case 'auth_error': S.auth.err(d); break;
      case 'char_error': { const e = $('#c-err'); if (e) e.textContent = 'Возраст персонажа — от 18 до 70 лет.'; break; }
      case 'inv': if (cur === 'inventory') S.inventory.render(d); break;
      case 'storage': if (cur === 'storage') S.storage.render(d); break;
      case 'bank': if (cur === 'bank') S.bank.upd(d); break;
      case 'quest': onQuest(d); break;
      case 'chapter': onChapter(d); break;
      case 'achievement': onAch(d); break;
      case 'queue': widget('w-queue', `<small class="mut">Электронная очередь</small><div class="big">А-${d.no}</div><span class="mut">Перед вами: ${d.ahead} чел.</span>`); break;
      case 'queue_call': widget('w-queue', `<small class="mut">Ваш номер вызван!</small><div class="big">Окно ${d.window}</div><span class="mut">Подойдите к паспортному столу</span>`, 'call'); setTimeout(() => widget('w-queue', ''), 60000); break;
      case 'form_error': S.passport_form.err(d); break;
      case 'progress': onProgress(d); break;
      case 'wounded': onWounded(d); break;
      case 'theory_result': S.theory.result(d); break;
      case 'call_state': S.phone.callState(d); break;
      case 'casino_result': S.casino.result(d); break;
    }
    if (key === 'passport_done' || (key === 'queue' && d.done)) widget('w-queue', '');
  });

  // ======================= клавиатура =======================
  document.addEventListener('keydown', (e) => {
    const typing = /INPUT|TEXTAREA|SELECT/.test((document.activeElement || {}).tagName || '');
    if (e.code === 'Escape') {
      if (ctxEl) return closeCtx();
      if (cur && !(S[cur].noClose)) { e.preventDefault(); close(); }
      chatOpen = false; return;
    }
    if (cur === 'fishing' && (e.code === 'Space' || e.code === 'Enter')) { e.preventDefault(); S.fishing.hit(); return; }
    if (cur === 'dialog' && /^Digit[1-4]$/.test(e.code)) { S.dialog.key(+e.code.slice(5)); return; }
    if (typing) return;
    if (!cur) {
      if (e.code === 'KeyT' || e.code === 'F6' || e.key === '`') { chatOpen = true; return; }
      if (e.code === 'Enter' && chatOpen) { chatOpen = false; return; }
      if (chatOpen) return;
      if (e.code === 'KeyI') send('inv', 'open');
      else if (e.code === 'KeyP') send('phone', 'open');
    } else if ((cur === 'inventory' && e.code === 'KeyI') || (cur === 'phone' && e.code === 'KeyP')) close();
  });
  document.addEventListener('mousedown', (e) => { if (ctxEl && !ctxEl.contains(e.target)) closeCtx(); });
  document.addEventListener('contextmenu', (e) => e.preventDefault());

  // ======================= демо-режим =======================
  if (DEMO) {
    const f = cef._fire;
    f('gj:data', 'items', JSON.stringify([{ id: 1, n: 'Пустая бутылка', k: 'bottle', t: 6 }, { id: 2, n: 'Шаурма', k: 'shawarma', t: 1 }, { id: 6, n: 'Телефон', k: 'phone', t: 3 }, { id: 8, n: 'Удочка', k: 'rod', t: 3 }, { id: 9, n: 'Наживка', k: 'bait', t: 0 }, { id: 13, n: 'Металлоискатель', k: 'detector', t: 3 }]));
    f('gj:hud', JSON.stringify({ cash: 1250, bank: 15400, hunger: 64, fatigue: 12, hp: 87, ar: 0, lvl: 3, exp: 5, need: 10, wanted: 2, speed: 87, fuel: 46, time: '21:37', id: 4, online: 57, det: 63 }));
    f('gj:data', 'quest', JSON.stringify({ active: true, quest: 'С самого дна', chapter: 1, chapterName: 'Пустые карманы', title: 'Собери 10 пустых бутылок', hint: 'Бутылки валяются возле вокзала Юнити. Подойди и нажми ALT.', progress: 4, target: 10 }));
    const demos = {
      '1': ['auth', { name: 'Ivan_Petrov', registered: false, server: 'Godjo Role Play', online: 57 }],
      '2': ['char', { m: [78, 79, 230], f: [77, 130, 196], name: 'Ivan_Petrov' }],
      '3': ['inventory', { w: 3600, wm: 25000, slots: [[0, 1, 7], [1, 2, 2], [2, 6, 1], [5, 8, 1], [6, 9, 25]] }],
      '4': ['menu', { title: 'Магазин 24/7', sub: 'Всё необходимое', menu: 'demo', items: [{ id: 1, t: 'Телефон', d: 'Звонки и SMS', p: 500, i: 'phone' }, { id: 2, t: 'Удочка', d: 'Для рыбалки на пирсе', p: 350, i: 'rod' }, { id: 3, t: 'Наживка ×10', d: '', p: 40, i: 'bait' }] }],
      '5': ['dialog', { npc: 'Дядя Вова', skin: 137, text: 'Ещё один местный, которого город выплюнул на обочину... Ничего, бывает. Садись к огню, погрейся.', options: [{ id: 1, t: 'Кто вы?' }, { id: 2, t: 'Мне нужна помощь' }, { id: 3, t: 'Отстань, старик' }] }],
      '6': ['bank', { cash: 1250, bank: 15400, atm: true, name: 'Ivan Petrov', card: '4276 1004 7919' }],
      '7': ['phone', { number: '5551234', cash: 1250, bank: 15400, contacts: [{ n: 'Дядя Вова', num: '5550101' }, { n: 'Экстренная служба', num: '911' }], sms: [{ f: '5550101', to: '5551234', t: 'Заходи, разговор есть.', d: '12.05 21:00' }] }],
      '8': ['casino', { cash: 12500, min: 100, max: 50000 }],
      '9': ['fishing', { zone: 20, speed: 6, time: 10, lvl: 1 }],
      '0': ['document', { type: 'passport', name: 'Ivan_Petrov', age: 25, sex: 0, number: '45 12 100013', city: 'Лос-Сантос', marital: 'Холост', issued: '12.05.2025', org: '—', lvl: 3 }],
      'q': ['theory', { questions: [{ q: 'Какая максимальная скорость в черте города?', a: ['60 км/ч', '90 км/ч', '110 км/ч'] }, { q: 'Что означает мигающий жёлтый сигнал?', a: ['Движение запрещено', 'Нерегулируемый перекрёсток', 'Можно ехать быстрее'] }], need: 1, time: 300 }],
      'w': ['passport_form', { name: 'Ivan_Petrov', age: 25, sex: 0, ticket: 12 }],
      'e': ['leader', { org: 'LSPD', bank: 250000, mats: 1200, myrank: 6, ranks: ['Кадет', 'Офицер', 'Сержант', 'Лейтенант', 'Капитан', 'Шеф'], members: [{ id: 1, n: 'Ivan_Petrov', r: 6, on: 1, last: '12.05.2025' }, { id: 2, n: 'Petr_Sidorov', r: 2, on: 0, last: '10.05.2025' }] }],
      'r': ['storage', { title: 'Шкаф в доме', inv: { slots: [[0, 1, 7], [1, 2, 2]] }, slots: [[0, 9, 40]], max: 40 }],
      't': ['prompt', { id: 'demo', title: 'Письмо', text: 'Пример окна подтверждения', input: true, ph: 'Введите текст', ok: 'Отправить' }]
    };
    console.log('Демо: клавиши 1-0, q, w, e, r, t открывают экраны; Esc закрывает');
    document.addEventListener('keydown', (e) => { if (cur || /INPUT|TEXTAREA|SELECT/.test(document.activeElement.tagName)) return; const d = demos[e.key]; if (d) open(d[0], d[1]); });
    if (location.hash.length > 1 && demos[location.hash.slice(1)]) { const d = demos[location.hash.slice(1)]; open(d[0], d[1]); }
    setTimeout(() => notify('ok', 'Добро пожаловать на Godjo Role Play!'), 300);
  }
})();
