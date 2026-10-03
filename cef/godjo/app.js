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

  // фокус браузера: без него CEF не получает мышь/клавиатуру и окна не кликаются
  function focus(on) { try { if (cef.set_focus) cef.set_focus(!!on); } catch (e) { } }
  function open(name, data) {
    if (!S[name]) return console.warn('нет экрана', name);
    closeCtx();
    if (cur && cur !== name && S[cur].close) S[cur].close(true);
    cur = name;
    layer.innerHTML = '';
    layer.classList.toggle("on", !S[name].noDim);
    document.body.classList.add("modal");
    focus(true);
    S[name].open(data);
  }
  function close(name, fromServer) {
    if (name && cur !== name) return;
    if (cur && S[cur].close) S[cur].close();
    cur = null; layer.innerHTML = ''; layer.classList.remove('on'); document.body.classList.remove('modal'); closeCtx(); focus(false);
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
      <div class="need" id="n-hp" style="--c:#ff5a5a">${ICO('heart', 'fill')}<div class="bar"><i></i></div></div>
      <div class="need" id="n-ar" style="--c:#5aa9ff">${ICO('shield')}<div class="bar"><i></i></div></div>
      <div class="need" id="n-hu" style="--c:#ff9f43">${ICO('bread')}<div class="bar"><i></i></div></div>
      <div class="need" id="n-fa" style="--c:#b98cff">${ICO('bolt')}<div class="bar"><i></i></div></div>
    </div>
    <div class="h-wep hidden" id="h-wep"></div>
    <div class="h-wanted" id="h-wanted"></div>`;
  const speedo = h(`<div class="speedo hidden"><div class="v" id="sp-v">0</div><div class="u">КМ/Ч</div><div class="f"><div class="bar"><i id="sp-f"></i></div><small id="sp-ft">100 л</small></div></div>`);
  const det = h(`<div class="det hidden"><div class="row" style="justify-content:space-between;margin-bottom:6px"><b>${ICO('radar')} Металлоискатель</b><span id="det-v">0%</span></div><div class="bar"><i id="det-b"></i></div></div>`);
  document.body.append(speedo, det);
  const WEAPONS = ['', 'Кастет', 'Клюшка', 'Дубинка', 'Нож', 'Бита', 'Лопата', 'Кий', 'Катана', 'Бензопила', 'Дилдо', 'Дилдо', 'Вибратор', 'Вибратор', 'Цветы', 'Трость',
    'Граната', 'Слезоточивый газ', 'Коктейль Молотова', '', '', '', 'Пистолет 9мм', 'Пистолет с глушителем', 'Desert Eagle', 'Дробовик', 'Обрез', 'Боевой дробовик', 'Micro Uzi', 'MP5', 'AK-47', 'M4', 'Tec-9',
    'Винтовка', 'Снайперская винтовка', 'РПГ', 'Ракетница', 'Огнемёт', 'Миниган', 'Взрывчатка', 'Детонатор', 'Баллончик', 'Огнетушитель', 'Фотоаппарат', 'ПНВ', 'Тепловизор', 'Парашют'];
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
    let st = ''; for (let i = 1; i <= 6; i++) st += `<span class="${i <= d.wanted ? 'on' : ''}">${ICO('star', 'fill')}</span>`;
    $('#h-wanted').innerHTML = d.wanted > 0 ? st : '';
    const wn = WEAPONS[d.wep] || (d.wep ? 'Оружие #' + d.wep : '');
    $('#h-wep').classList.toggle('hidden', !d.wep);
    if (d.wep) $('#h-wep').innerHTML = `${ICO('gun')}<b>${esc(wn)}</b>${d.wep >= 16 && d.wep !== 46 && d.ammo > 0 ? `<span>${d.ammo}</span>` : ''}`;
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
    const a = h(`<div class="ach"><small>${ICO('trophy')} Достижение</small><div>${esc(d.title)}</div></div>`);
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
  const ORIGINS = [[ICO('city'), 'Местный', 'Вырос в Лос-Сантосе, знает улицы'], [ICO('wheat'), 'Из деревни', 'Приехал за мечтой, крепкие руки'], [ICO('chain'), 'Бывший заключённый', 'Только вышел, начинает заново'], [ICO('briefcase'), 'Бывший бизнесмен', 'Прогорел, но голова на месте']];
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
      $('#c-skins').innerHTML = list.map((s, i) => `<div class="card ${i === this.idx ? 'on' : ''}" data-i="${i}"><span class="big">${this.sex ? ICO('woman') : ICO('man')}</span>${names[i]}<br><small class="mut">#${s}</small></div>`).join('');
      $$('#c-skins .card').forEach(c => c.onclick = () => { this.idx = +c.dataset.i; $$('#c-skins .card').forEach(x => x.classList.toggle('on', x === c)); this.pick(); });
      $('#c-note').textContent = this.sex ? 'В GTA San Andreas только один женский скин бездомной (#77), поэтому два других — максимально скромные городские образы.' : 'Вращайте камеру — персонаж показан в игре справа.';
    },
    pick() { send('char', 'skin', this.sex, this.idx); }
  };

  // ---------- меню ----------
  const ICONS = { cash: ICO('cash'), fish: ICO('fish'), detector: ICO('radar'), shovel: ICO('shovel'), car: ICO('car'), key: ICO('keyboard'), quest: ICO('scroll'), chat: ICO('chat'), doc: ICO('idcard'), home: ICO('home'), org: ICO('landmark'), family: ICO('users'), info: ICO('info'), gps: ICO('pin'), food: ICO('burger'), drink: ICO('cup'), phone: ICO('phone'), shirt: ICO('shirt'), hat: ICO('cap'), job: ICO('toolbox'), med: ICO('pill'), gun: ICO('gun'), bank: ICO('bank'), star: ICO('star', 'fill'), tune: ICO('wrench'), fuel: ICO('fuel'), bag: ICO('bag'), ticket: ICO('ticket'), heart: ICO('heart', 'fill') };
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
      const w = h(`<div class="dlg"><div class="who"><div class="ava">${ICO(d.ava || 'oldman')}</div><div><b>${esc(d.npc)}</b><div class="mut" style="font-size:12px">нажмите на ответ или 1–4</div></div></div><div class="text"></div><div class="opts"></div></div>`);
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
  const KEYICON = { bottle: ICO('bottle'), shawarma: ICO('wrap'), burger: ICO('burger'), water: ICO('drop'), chips: ICO('chips'), phone: ICO('phone'), energy: ICO('can'), rod: ICO('rod'), bait: ICO('worm'), fish_s: ICO('fish'), fish_m: ICO('fish'), fish_b: ICO('fish'), detector: ICO('radar'), shovel: ICO('shovel'), coin: ICO('coin'), ring: ICO('ring'), junk: ICO('nut'), medkit: ICO('medkit'), repair: ICO('wrench'), canister: ICO('fuel'), ore: ICO('rock'), colt: ICO('gun'), deagle: ICO('gun'), shotgun: ICO('gun'), m4: ICO('gun'), ak47: ICO('gun'), baton: ICO('bat'), ammo: ICO('bomb'), armour: ICO('vest'), letter: ICO('mail'), cigs: ICO('cig'), lockpick: ICO('key') };
  function ITEM_ICON(key) { return KEYICON[key] || ''; }
  const itemIcon = (id) => { const it = ITEMS[id]; return it ? (KEYICON[it.k] || ICO('box')) : ICO('help'); };
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
        if (it.t !== 6) acts.push([ICO('hand') + ' Использовать', () => send('inv', 'use', s[0])]);
        acts.push([ICO('handshake') + ' Передать рядом', () => askAmount('Сколько передать?', s[2], (n) => send('inv', 'give', s[0], n))]);
        acts.push([ICO('trash') + ' Выбросить', () => askAmount('Сколько выбросить?', s[2], (n) => send('inv', 'drop', s[0], n))]);
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
      if (d.type === 'passport') body = `<div class="dh"><b>ПАСПОРТ ГРАЖДАНИНА SAN ANDREAS</b><span>${esc(d.number)}</span></div><div class="db"><div class="photo">${d.sex ? ICO('woman') : ICO('man')}</div><div class="fields">
          <div class="full"><small>Фамилия, имя</small><b>${nm}</b></div><div><small>Возраст</small><b>${d.age}</b></div><div><small>Пол</small><b>${d.sex ? 'Женский' : 'Мужской'}</b></div>
          <div><small>Место рождения</small><b>${esc(d.city)}</b></div><div><small>Семейное положение</small><b>${esc(d.marital)}</b></div>
          <div><small>Организация</small><b>${esc(d.org)}</b></div><div><small>Дата выдачи</small><b>${esc(d.issued)}</b></div>
          <div class="full"><small>Подпись</small><span class="sign">${nm}</span></div></div></div>`;
      else if (d.type === 'med') body = `<div class="dh"><b>МЕДИЦИНСКАЯ КАРТА</b><span>All Saints General</span></div><div class="db"><div class="photo">${ICO('medic')}</div><div class="fields"><div class="full"><small>Пациент</small><b>${nm}</b></div><div><small>Возраст</small><b>${d.age}</b></div><div><small>Заключение</small><b>${esc(d.status)}</b></div><div class="full"><small>Годен к работе и управлению ТС</small><b>Да</b></div></div></div>`;
      else body = `<div class="dh"><b>ЛИЦЕНЗИИ</b><span>DMV San Andreas</span></div><div class="db"><div class="photo">${ICO('car')}</div><div class="fields"><div class="full"><small>Владелец</small><b>${nm}</b></div><div><small>Теория ПДД</small><b>${d.theory ? ICO('check') + ' Сдана' : ICO('x') + ' Нет'}</b></div><div><small>Права кат. B</small><b>${d.drive ? ICO('check') + ' Есть' : ICO('x') + ' Нет'}</b></div></div></div>`;
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
      $('.win-b', this.w).innerHTML = `<div style="text-align:center;padding:20px"><div style="font-size:60px">${r.passed ? ICO('party') : ICO('sad')}</div><h2 style="margin:10px 0">${r.passed ? 'Экзамен сдан!' : 'Экзамен не сдан'}</h2><div class="mut">Правильных ответов: <b class="${r.passed ? 'green-t' : 'red-t'}">${r.right} из ${r.total}</b></div></div>`;
      $('.win-f', this.w).innerHTML = ''; const c = h(`<button class="btn gold">Закрыть</button>`); c.onclick = () => close(); $('.win-f', this.w).append(c);
    },
    close() { clearInterval(this.t); this.w = null; }
  };

  // ---------- телефон ----------
  S.phone = {
    noDim: true,
    open(d) {
      this.d = d; this.view = 'home'; this.dial = '';
      this.w = h(`<div class="phone"><div class="scr"><div class="sbar"><span id="ph-time">${esc(HUD.time || '')}</span><span>${ICO('signal')} Godjo Mobile</span></div><div class="papp"></div><button class="homebar" title="Закрыть (ESC / P)"><i></i></button></div><button class="ph-x" title="Закрыть телефон">×</button></div>`);
      $('.homebar', this.w).onclick = () => { if (this.view !== 'home') this.go('home'); else close('phone'); };
      $('.ph-x', this.w).onclick = () => close('phone');
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
        [[ICO('call'), 'Телефон', '#2ecc71', 'dial'], [ICO('users'), 'Контакты', '#3498db', 'contacts'], [ICO('chat'), 'Сообщения', '#9b59b6', 'sms'], [ICO('taxi'), 'Такси', '#f1c40f', 'taxi'], [ICO('bank'), 'Банк', '#e67e22', 'bank'], [ICO('ambulance'), '911', '#e74c3c', '911']].forEach(([i, n, c, v]) => {
          const e = h(`<div class="app"><i style="--c:${c}">${i}</i>${n}</div>`);
          e.onclick = () => { if (v === 'taxi') { send('phone', 'taxi'); notify('info', 'Вызов такси отправлен диспетчеру.'); } else if (v === '911') send('phone', 'call', '911'); else this.go(v); };
          apps.append(e);
        });
        a.append(apps);
      } else if (this.view === 'dial') {
        a.append(this.head('Телефон'));
        const disp = h(`<div class="dial">${esc(this.dial) || '<span class="mut">номер</span>'}</div>`), kp = h(`<div class="keypad"></div>`);
        ['1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '0', ICO('backspace')].forEach(k => { const b = h(`<button>${k}</button>`); b.onclick = () => { this.dial = k === ICO('backspace') ? this.dial.slice(0, -1) : (this.dial + k).slice(0, 10); disp.innerHTML = esc(this.dial) || '<span class="mut">номер</span>'; }; kp.append(b); });
        const c = h(`<button class="callbtn">${ICO('call')}</button>`); c.onclick = () => { if (this.dial) send('phone', 'call', this.dial.replace(/\D/g, '')); };
        a.append(disp, kp, c);
      } else if (this.view === 'contacts') {
        a.append(this.head('Контакты'));
        const l = h(`<div class="plist"></div>`);
        (d.contacts || []).forEach(c => {
          const e = h(`<div class="pitem"><div class="grow"><b>${esc(c.n)}</b><small>${esc(c.num)}</small></div><button title="Позвонить">${ICO('call')}</button><button title="SMS">${ICO('chat')}</button>${/^(5550101|911)$/.test(c.num) ? '' : '<button title="Удалить">' + ICO('trash') + '</button>'}</div>`);
          const bs = $$('button', e); bs[0].onclick = () => send('phone', 'call', c.num); bs[1].onclick = () => { this.smsTo = c.num; this.go('write'); };
          if (bs[2]) bs[2].onclick = () => { send('phone', 'delc', c.num); d.contacts = d.contacts.filter(x => x !== c); this.render(); };
          l.append(e);
        });
        const add = h(`<div class="col" style="margin-top:12px"><input id="pc-n" placeholder="Имя" maxlength="24"><input id="pc-num" placeholder="Номер" maxlength="10"><button class="btn gold">Добавить контакт</button></div>`);
        $('button', add).onclick = () => { const n = $('#pc-n').value.trim(), num = $('#pc-num').value.replace(/\D/g, ''); if (!n || !num) return; send('phone', 'addc', num, n); d.contacts.push({ n, num }); this.render(); };
        a.append(l, add);
      } else if (this.view === 'sms') {
        a.append(this.head('Сообщения'));
        const nb = h(`<button class="btn gold wide" style="margin-bottom:10px">${ICO('pencil')} Новое сообщение</button>`); nb.onclick = () => { this.smsTo = ''; this.go('write'); };
        const l = h(`<div class="col" style="gap:2px"></div>`);
        (d.sms || []).forEach(m => { const out = m.f === d.number; l.append(h(`<div class="sms ${out ? 'out' : 'in'}"><b style="font-size:11px">${out ? ICO('arrow') + ' ' + esc(m.to) : esc(m.f)}</b><br>${esc(m.t)}<small>${esc(m.d)}</small></div>`)); });
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
        if (st.state === 'incoming') widget('w-call', `${ICO('call')} Входящий: <b>${esc(st.num)}</b><br><span class="mut">/pickup — ответить, /hangup — сбросить или P</span>`, 'call');
        else widget('w-call', st.state === 'talking' ? ICO('call') + ' Идёт разговор · пишите в чат · /hangup' : ICO('call') + ' Вызов...', 'call');
        return;
      }
      if (!c) { c = h(`<div class="callscr"></div>`); $('.scr', this.w).append(c); }
      const lab = { incoming: 'Входящий вызов', ringing: 'Вызов...', talking: 'Идёт разговор — пишите в чат' }[st.state] || '';
      c.innerHTML = `<div style="font-size:60px">${ICO('user')}</div><div class="nm">${esc(st.num || this.dial || '')}</div><div class="mut">${lab}</div><div class="row" style="gap:30px;margin-top:20px">${st.state === 'incoming' ? '<button class="callbtn" data-a>' + ICO('call') + '</button>' : ''}<button class="callbtn hang" data-h>${ICO('calloff')}</button></div>`;
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
  const SYM = [ICO('cherry'), ICO('lemon'), ICO('peach'), ICO('bell'), ICO('gem'), ICO('seven')];
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
      b.innerHTML = `<div class="tabs"><button data-g="roulette" class="${this.game === 'roulette' ? 'on' : ''}">${ICO('wheel')} Рулетка</button><button data-g="slots" class="${this.game === 'slots' ? 'on' : ''}">${ICO('slot')} Слоты</button></div><div id="cs-body"></div>
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
        body.innerHTML = `<div class="reels">${[0, 1, 2].map(() => `<div class="reel">${SYM[5]}</div>`).join('')}</div><div class="mut" style="text-align:center">Три ${ICO('seven')} — ×100 · три ${ICO('gem')} — ×25 · три ${ICO('bell')} — ×15 · две ${ICO('cherry')} подряд — ×2 · пара — возврат</div>`;
      }
      $('#cs-go', b).onclick = () => {
        if (this.busy) return;
        const a = parseInt($('#cs-amt').value) || 0; this.amt = a;
        if (a < this.d.min || a > this.d.max) return notify('error', `Ставка от ${money(this.d.min)} до ${money(this.d.max)}`);
        this.busy = true; $('#cs-res').textContent = '';
        if (this.game === 'roulette') { $('#cs-ball').className = 'ball spin'; send('casino', 'roulette', this.bet.type, this.bet.val, a); }
        else { $$('.reel').forEach(r => r.classList.add('spin')); this.spinT = setInterval(() => $$('.reel').forEach(r => r.innerHTML = SYM[Math.random() * 6 | 0]), 80); send('casino', 'slots', a); }
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
        r.reels.forEach((s, i) => setTimeout(() => { if (i === 2) clearInterval(this.spinT); if (reels[i]) { reels[i].classList.remove('spin'); reels[i].innerHTML = SYM[s]; } if (i === 2) { reels.forEach((x, k) => x.innerHTML = SYM[r.reels[k]]); fin(); } }, 700 + i * 450));
      }
    },
    close() { clearInterval(this.spinT); this.w = null; }
  };

  // ---------- рыбалка ----------
  S.fishing = {
    noDim: true,
    open(d) {
      this.d = d; this.hits = 0; this.miss = 0; this.need = 3; this.pos = 0; this.dir = 1; this.left = (d.time || 10) * 10;
      this.w = h(`<div class="fish"><b style="font-size:18px">${ICO('rod')} Клюёт!</b><div class="mut" style="font-size:12px;margin-top:4px">Жмите <span class="kbd">ПРОБЕЛ</span> или кликайте, когда метка в зелёной зоне. Нужно ${this.need} попадания, ошибок — не больше 2.</div>
        <div class="track"><div class="zone"></div><div class="mk"></div></div><div class="hits"></div><div class="mut" id="fi-t" style="font-size:12px;margin-top:6px"></div></div>`);
      layer.append(this.w);
      this.newZone(); this.draw();
      this.t = setInterval(() => this.step(), 16);
      this.tt = setInterval(() => { this.left--; $('#fi-t').textContent = 'Осталось: ' + (this.left / 10).toFixed(1) + ' с'; if (this.left <= 0) this.finish(false); }, 100);
      this.w.onclick = () => this.hit();
    },
    newZone() { const z = this.d.zone || 20; this.zs = 5 + Math.random() * (90 - z); this.ze = this.zs + z; const e = $('.zone', this.w); e.style.left = this.zs + '%'; e.style.width = z + '%'; },
    step() { const sp = 0.6 + (10 - (this.d.speed || 6)) * 0.12 + this.hits * 0.15; this.pos += this.dir * sp; if (this.pos >= 100) { this.pos = 100; this.dir = -1; } if (this.pos <= 0) { this.pos = 0; this.dir = 1; } $('.mk', this.w).style.left = 'calc(' + this.pos + '% - 3px)'; },
    draw() { $('.hits', this.w).innerHTML = Array.from({ length: this.need }, (_, i) => i < this.hits ? ICO('fish') : ICO('dot')).join('') + ' ' + ICO('x').repeat(this.miss); },
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


  // ======================= настройки интерфейса =======================
  const SET_DEF = { accent: '#FFC94D', scale: 100, side: 'right', opacity: 100, top: true, money: true, lvl: true, needs: true, wep: true, wanted: true, quest: true, speedo: true, notify: 'top' };
  let SET = (() => { try { return Object.assign({}, SET_DEF, JSON.parse(localStorage.getItem('gj_set') || '{}')); } catch (e) { return Object.assign({}, SET_DEF); } })();
  const ACCENTS = [['#FFC94D', 'Золото'], ['#5AA9FF', 'Небо'], ['#6EE07A', 'Мята'], ['#FF5A5A', 'Рубин'], ['#B98CFF', 'Аметист'], ['#FF8A3D', 'Апельсин'], ['#3DE0D0', 'Бирюза'], ['#F5F5F7', 'Платина']];
  function shade(hex, k) {
    const n = parseInt(hex.slice(1), 16); const f = (c) => Math.max(0, Math.min(255, Math.round(c * (1 + k))));
    return '#' + [f(n >> 16), f((n >> 8) & 255), f(n & 255)].map(c => c.toString(16).padStart(2, '0')).join('');
  }
  function applySet() {
    const r = document.documentElement.style, b = document.body.classList;
    r.setProperty('--gold', SET.accent); r.setProperty('--gold2', shade(SET.accent, -0.12));
    r.setProperty('--hud-scale', SET.scale / 100); r.setProperty('--hud-op', SET.opacity / 100);
    b.toggle('hud-left', SET.side === 'left');
    b.toggle('nt-bottom', SET.notify === 'bottom');
    ['top', 'money', 'lvl', 'needs', 'wep', 'wanted', 'quest', 'speedo'].forEach(k => b.toggle('no-' + k, !SET[k]));
  }
  function saveSet() { try { localStorage.setItem('gj_set', JSON.stringify(SET)); } catch (e) { } applySet(); }
  applySet();
  S.settings = {
    noDim: true,
    open() {
      const tg = (k, t) => `<label class="sw"><input type="checkbox" data-k="${k}" ${SET[k] ? 'checked' : ''}><i></i><span>${t}</span></label>`;
      const body = h(`<div class="set">
        <div class="set-s"><small class="mut">Цвет акцента</small><div class="acc">${ACCENTS.map(([c, n]) => `<button data-c="${c}" title="${n}" style="--c:${c}" class="${c.toLowerCase() === SET.accent.toLowerCase() ? 'on' : ''}"></button>`).join('')}<input type="color" id="st-col" value="${SET.accent}" title="Свой цвет"></div></div>
        <div class="set-s"><small class="mut">Размер HUD: <b id="st-sv">${SET.scale}%</b></small><input type="range" id="st-sc" min="70" max="140" step="5" value="${SET.scale}"></div>
        <div class="set-s"><small class="mut">Прозрачность HUD: <b id="st-ov">${SET.opacity}%</b></small><input type="range" id="st-op" min="40" max="100" step="5" value="${SET.opacity}"></div>
        <div class="set-s row"><div class="grow"><small class="mut">HUD</small><div class="seg" data-g="side"><button data-v="right">Справа</button><button data-v="left">Слева</button></div></div>
          <div class="grow"><small class="mut">Уведомления</small><div class="seg" data-g="notify"><button data-v="top">Сверху</button><button data-v="bottom">Снизу</button></div></div></div>
        <div class="set-s"><small class="mut">Что показывать</small><div class="sws">
          ${tg('top', 'Время, ID, онлайн')}${tg('money', 'Деньги и банк')}${tg('lvl', 'Уровень и опыт')}${tg('needs', 'Здоровье, голод, усталость')}
          ${tg('wep', 'Оружие и патроны')}${tg('wanted', 'Звёзды розыска')}${tg('quest', 'Задание квеста')}${tg('speedo', 'Спидометр')}</div></div>
      </div>`);
      const w = win('settings', `${ICO('wrench')} Настройки интерфейса`, 'Сохраняются на этом компьютере', body, true);
      const reset = h(`<button class="btn">Сбросить</button>`), ok = h(`<button class="btn gold">Готово</button>`);
      $('.win-f', w).append(reset, ok);
      const seg = () => $$('.seg', body).forEach(g => $$('button', g).forEach(x => x.classList.toggle('on', SET[g.dataset.g] === x.dataset.v)));
      seg();
      $$('.acc button', body).forEach(x => x.onclick = () => { SET.accent = x.dataset.c; $$('.acc button', body).forEach(y => y.classList.toggle('on', y === x)); $('#st-col').value = x.dataset.c; saveSet(); });
      $('#st-col').oninput = (e) => { SET.accent = e.target.value; $$('.acc button', body).forEach(y => y.classList.remove('on')); saveSet(); };
      $('#st-sc').oninput = (e) => { SET.scale = +e.target.value; $('#st-sv').textContent = SET.scale + '%'; saveSet(); };
      $('#st-op').oninput = (e) => { SET.opacity = +e.target.value; $('#st-ov').textContent = SET.opacity + '%'; saveSet(); };
      $$('.seg button', body).forEach(x => x.onclick = () => { SET[x.parentNode.dataset.g] = x.dataset.v; seg(); saveSet(); });
      $$('[data-k]', body).forEach(x => x.onchange = () => { SET[x.dataset.k] = x.checked; saveSet(); });
      reset.onclick = () => { SET = Object.assign({}, SET_DEF); saveSet(); open('settings', {}); };
      ok.onclick = () => close();
    }
  };

  // ======================= мини-игры =======================
  // d: {type, lvl, token, title, hint, seed}. Результат: send('mini', token, 1|0)
  const ARW = { ArrowUp: '↑', ArrowDown: '↓', ArrowLeft: '←', ArrowRight: '→' };
  const shuffle = (a) => { for (let i = a.length - 1; i > 0; i--) { const j = Math.floor(Math.random() * (i + 1)); [a[i], a[j]] = [a[j], a[i]]; } return a; };
  S.mini = {
    open(d) {
      if (this.timers) this.stop(); if (this._ku) { document.removeEventListener('keyup', this._ku); this._ku = null; }
      this.d = d; this.done = false; this.sent = true; this.timers = []; this.keyH = null; this.raf = 0;
      const lvl = Math.max(1, Math.min(5, d.lvl | 0));
      this.w = win('mini', esc(d.title || 'Мини-игра'), esc(d.hint || ''), '<div class="mg"></div><div class="mg-st mut"></div>', false);
      this.box = $('.mg', this.w); this.st = $('.mg-st', this.w);
      const g = this['g_' + d.type]; if (g) g.call(this, lvl); else this.finish(true);
    },
    status(t) { if (this.st) this.st.innerHTML = t; },
    every(fn, ms) { const t = setInterval(fn, ms); this.timers.push(t); return t; },
    later(fn, ms) { const t = setTimeout(fn, ms); this.timers.push(t); return t; },
    loop(fn) { const step = (ts) => { if (this.done) return; fn(ts); this.raf = requestAnimationFrame(step); }; this.raf = requestAnimationFrame(step); },
    stop() { this.timers.forEach(t => { clearInterval(t); clearTimeout(t); }); this.timers = []; cancelAnimationFrame(this.raf); this.keyH = null; },
    finish(ok) {
      if (this.done) return; this.done = true; this.stop();
      if (this.box) this.box.classList.add(ok ? 'win-ok' : 'win-bad');
      this.status(ok ? '<b class="green-t">Отлично!</b>' : '<b class="red-t">Не получилось</b>');
      this.res = ok ? 1 : 0; this.sent = false; this.later2 = setTimeout(() => this.flush(), 600);
    },
    flush() { if (this.sent || !this.d) return; this.sent = true; send('mini', this.d.token, this.res); },
    key(e) { return this.keyH ? this.keyH(e) : false; },
    close() { this.stop(); if (!this.done) { this.done = true; this.res = 0; this.sent = false; } clearTimeout(this.later2); this.flush(); this.w = null; },

    // поймать бегунок в зелёной зоне (пробел / клик)
    g_timing(lvl) {
      const need = lvl > 2 ? 2 : 1; let hits = 0, miss = 0, pos = 0, dir = 1, zs, zw;
      this.box.innerHTML = '<div class="mg-bar"><div class="mg-zone"></div><div class="mg-cur"></div></div><button class="btn gold wide mg-go">ПРОБЕЛ / клик</button>';
      const zone = $('.mg-zone', this.box), cur = $('.mg-cur', this.box);
      const place = () => { zw = Math.max(8, 22 - lvl * 3); zs = 10 + Math.random() * (80 - zw); zone.style.left = zs + '%'; zone.style.width = zw + '%'; };
      place(); const sp = 0.6 + lvl * 0.25;
      this.loop(() => { pos += dir * sp; if (pos >= 100) { pos = 100; dir = -1; } if (pos <= 0) { pos = 0; dir = 1; } cur.style.left = pos + '%'; });
      const hit = () => { if (this.done) return; if (pos >= zs && pos <= zs + zw) { hits++; if (hits >= need) return this.finish(true); place(); } else if (++miss >= 3) return this.finish(false); this.status(`Попаданий: ${hits}/${need} · промахов: ${miss}/3`); };
      $('.mg-go', this.box).onclick = hit; this.keyH = (e) => { if (e.code === 'Space' || e.key === ' ' || e.keyCode === 32) { hit(); return true; } return false; };
      this.status(`Попаданий: 0/${need}`);
    },
    // быстро жать пробел
    g_mash(lvl) {
      let v = 0, left = 8; const goal = 100, inc = Math.max(4, 9 - lvl);
      this.box.innerHTML = `<div class="mg-bar fill"><i></i></div><button class="btn gold wide mg-go">Жмите ПРОБЕЛ или кликайте!</button>`;
      const bar = $('.mg-bar i', this.box);
      const push = () => { if (this.done) return; v = Math.min(goal, v + inc); bar.style.width = v + '%'; if (v >= goal) this.finish(true); };
      this.every(() => { v = Math.max(0, v - (1 + lvl * 0.4)); bar.style.width = v + '%'; }, 100);
      this.every(() => { left--; this.status(`Осталось: ${left} с`); if (left <= 0) this.finish(false); }, 1000);
      $('.mg-go', this.box).onclick = push; this.keyH = (e) => { if (e.code === 'Space' || e.keyCode === 32) { if (!e.repeat) push(); return true; } return false; };
      this.status(`Осталось: ${left} с`);
    },
    // удержать и отпустить в зелёной зоне
    g_hold(lvl) {
      let v = 0, holding = false, tries = 3; const zs = 62 - lvl * 2, zw = Math.max(8, 20 - lvl * 2);
      this.box.innerHTML = `<div class="mg-bar fill"><div class="mg-zone" style="left:${zs}%;width:${zw}%"></div><i></i></div><button class="btn gold wide mg-go">Зажмите и держите</button>`;
      const bar = $('.mg-bar i', this.box), b = $('.mg-go', this.box);
      const release = () => { if (!holding || this.done) return; holding = false; if (v >= zs && v <= zs + zw) return this.finish(true); if (--tries <= 0) return this.finish(false); v = 0; bar.style.width = '0%'; this.status(`Мимо! Попыток: ${tries}`); };
      b.onmousedown = () => { if (!this.done) { holding = true; v = 0; } };
      b.onmouseup = release; b.onmouseleave = release;
      this.keyH = (e) => { if (e.code === 'Space' || e.keyCode === 32) { if (e.type === 'keydown' && !holding) { holding = true; v = 0; } return true; } return false; };
      this._ku = (e) => { if (e.code === 'Space' || e.keyCode === 32) release(); }; document.addEventListener('keyup', this._ku);
      this.loop(() => { if (holding) { v += 0.5 + lvl * 0.2; if (v >= 100) { v = 100; release(); } bar.style.width = v + '%'; } });
      this.status(`Попыток: ${tries}`);
    },
    // соединить провода одного цвета
    g_wires(lvl) {
      const COLS = ['#ff5a5a', '#5aa9ff', '#6ee07a', '#ffc94d', '#c77dff', '#ff9f43'];
      const n = Math.min(6, 3 + lvl), cols = COLS.slice(0, n), right = shuffle(cols.slice()); let sel = null, ok = 0, err = 0;
      this.box.innerHTML = '<div class="mg-wires"><div class="l"></div><div class="mid"></div><div class="r"></div></div>';
      const L = $('.l', this.box), R = $('.r', this.box);
      cols.forEach(c => { const e = h(`<button class="wire" style="--c:${c}"></button>`); e.onclick = () => { if (e.disabled) return; $$('.wire.sel', L).forEach(x => x.classList.remove('sel')); e.classList.add('sel'); sel = { c, e }; }; L.append(e); });
      right.forEach(c => { const e = h(`<button class="wire" style="--c:${c}"></button>`); e.onclick = () => {
        if (!sel || e.disabled || this.done) return;
        if (sel.c === c) { sel.e.disabled = e.disabled = true; sel.e.classList.remove('sel'); sel.e.classList.add('done'); e.classList.add('done'); sel = null; if (++ok >= n) this.finish(true); }
        else { err++; e.classList.add('bad'); setTimeout(() => e.classList.remove('bad'), 300); if (err >= 3) this.finish(false); }
        this.status(`Соединено: ${ok}/${n} · ошибок: ${err}/3`);
      }; R.append(e); });
      this.status(`Соединено: 0/${n}`);
    },
    // повторить последовательность стрелок
    g_seq(lvl) {
      const keys = Object.keys(ARW), len = 3 + lvl, seq = Array.from({ length: len }, () => keys[Math.floor(Math.random() * 4)]); let pos = 0, show = true, retry = 1;
      this.box.innerHTML = '<div class="mg-seq"></div><div class="mg-pad"></div>';
      const S2 = $('.mg-seq', this.box), pad = $('.mg-pad', this.box);
      const demo = () => { show = true; pos = 0; S2.innerHTML = ''; this.status('Запоминайте...'); seq.forEach((k, i) => this.later(() => { S2.innerHTML = `<span class="big">${ARW[k]}</span>`; this.later(() => S2.innerHTML = '', 450); }, 300 + i * 650)); this.later(() => { show = false; S2.innerHTML = seq.map(() => '<i>•</i>').join(''); this.status('Повторите стрелками на клавиатуре или кнопками'); }, 300 + len * 650); };
      const press = (k) => { if (show || this.done) return; if (k === seq[pos]) { $$('i', S2)[pos].textContent = ARW[k]; $$('i', S2)[pos].className = 'ok'; if (++pos >= len) this.finish(true); } else if (retry-- > 0) { this.status('<b class="red-t">Ошибка!</b> Ещё раз...'); this.later(demo, 700); show = true; } else this.finish(false); };
      ['ArrowLeft', 'ArrowUp', 'ArrowDown', 'ArrowRight'].forEach(k => { const b = h(`<button class="btn">${ARW[k]}</button>`); b.onclick = () => press(k); pad.append(b); });
      this.keyH = (e) => { const map = { 37: 'ArrowLeft', 38: 'ArrowUp', 39: 'ArrowRight', 40: 'ArrowDown' }; const k = ARW[e.key] ? e.key : map[e.keyCode]; if (k) { press(k); return true; } return false; };
      demo();
    },
    // таблица окулиста
    g_eye(lvl) {
      const AB = 'ШБМНКЫИПВ'.split(''); const rows = [1, 2, 3, 4, 5, 6].map(n => Array.from({ length: n + 1 }, () => AB[Math.floor(Math.random() * AB.length)]));
      let round = 0, good = 0; const total = 4, need = 3;
      this.box.innerHTML = `<div class="mg-eye">${rows.map((r, i) => `<div style="font-size:${40 - i * 6}px">${r.map(c => `<span>${c}</span>`).join('')}</div>`).join('')}</div><div class="mg-pad"></div>`;
      const spans = $$('.mg-eye span', this.box), pad = $('.mg-pad', this.box);
      const next = () => {
        if (round >= total) return this.finish(good >= need);
        spans.forEach(s => s.classList.remove('pt'));
        const rowI = Math.min(5, 1 + round + Math.floor(lvl / 2)), row = $$('.mg-eye > div', this.box)[rowI], ss = $$('span', row), t = ss[Math.floor(Math.random() * ss.length)]; t.classList.add('pt');
        const ans = t.textContent, opts = shuffle([ans, ...shuffle(AB.filter(c => c !== ans)).slice(0, 3)]);
        pad.innerHTML = ''; opts.forEach(o => { const b = h(`<button class="btn">${o}</button>`); b.onclick = () => { if (o === ans) good++; round++; this.status(`Верно: ${good}/${round}`); next(); }; pad.append(b); });
      };
      this.status('Какая буква под указкой?'); next();
    },
    // поймать кадр: лицо в рамке
    g_photo(lvl) {
      let tries = 3, t0 = performance.now();
      this.box.innerHTML = `<div class="mg-photo"><div class="frame"></div><div class="face">${ICO('user')}</div></div><button class="btn gold wide mg-go">${ICO('target')} Снимок (ПРОБЕЛ)</button>`;
      const face = $('.face', this.box); let fx = 0, fy = 0; const sp = 0.0012 + lvl * 0.0005;
      this.loop((ts) => { const t = (ts - t0) * sp; fx = Math.sin(t * 1.3) * 90 + Math.sin(t * 3.1) * 25; fy = Math.cos(t * 1.7) * 45; face.style.transform = `translate(calc(-50% + ${fx}px), calc(-50% + ${fy}px))`; });
      const shot = () => { if (this.done) return; this.box.classList.add('flash'); setTimeout(() => this.box && this.box.classList.remove('flash'), 150); if (Math.abs(fx) < 16 - lvl && Math.abs(fy) < 14 - lvl) return this.finish(true); if (--tries <= 0) return this.finish(false); this.status(`Смазано! Попыток: ${tries}`); };
      $('.mg-go', this.box).onclick = shot; this.keyH = (e) => { if (e.code === 'Space' || e.keyCode === 32) { shot(); return true; } return false; };
      this.status(`Попыток: ${tries}`);
    },
    // подпись мышью
    g_sign() {
      this.box.innerHTML = '<canvas class="mg-sign" width="460" height="170"></canvas><div class="row"><button class="btn mg-clr">Стереть</button><div class="grow"></div><button class="btn gold mg-ok" disabled>Готово</button></div>';
      const cv = $('canvas', this.box), cx = cv.getContext('2d'); let draw = false, len = 0, lx = 0, ly = 0;
      cx.strokeStyle = '#1b3a8a'; cx.lineWidth = 2.5; cx.lineCap = 'round';
      const p = (e) => { const r = cv.getBoundingClientRect(); return [(e.clientX - r.left) * cv.width / r.width, (e.clientY - r.top) * cv.height / r.height]; };
      cv.onmousedown = (e) => { draw = true; [lx, ly] = p(e); };
      cv.onmousemove = (e) => { if (!draw) return; const [x, y] = p(e); cx.beginPath(); cx.moveTo(lx, ly); cx.lineTo(x, y); cx.stroke(); len += Math.hypot(x - lx, y - ly); lx = x; ly = y; if (len > 250) $('.mg-ok', this.box).disabled = false; };
      cv.onmouseup = cv.onmouseleave = () => draw = false;
      $('.mg-clr', this.box).onclick = () => { cx.clearRect(0, 0, cv.width, cv.height); len = 0; $('.mg-ok', this.box).disabled = true; };
      $('.mg-ok', this.box).onclick = () => this.finish(true);
    },
    // запомнить код
    g_code(lvl) {
      const len = 3 + lvl; let tries = 2; const code = Array.from({ length: len }, () => Math.floor(Math.random() * 10)).join('');
      this.box.innerHTML = `<div class="mg-code big"></div><div class="row"><input class="mg-in" maxlength="${len}" placeholder="Введите код" disabled><button class="btn gold mg-ok" disabled>OK</button></div>`;
      const c = $('.mg-code', this.box), inp = $('.mg-in', this.box), ok = $('.mg-ok', this.box);
      const showC = () => { c.textContent = code; inp.disabled = ok.disabled = true; this.status('Запомните код...'); this.later(() => { c.textContent = '•'.repeat(len); inp.disabled = ok.disabled = false; inp.value = ''; inp.focus(); this.status(`Попыток: ${tries}`); }, 1600 + len * 350); };
      const check = () => { if (inp.value.replace(/\D/g, '') === code) return this.finish(true); if (--tries <= 0) return this.finish(false); this.status('<b class="red-t">Неверно</b>, смотрите ещё раз'); showC(); };
      ok.onclick = check; inp.onkeydown = (e) => { if (e.key === 'Enter' || e.keyCode === 13) check(); };
      showC();
    },
    // найти предметы
    g_find(lvl) {
      const cells = 20, n = Math.max(1, Math.min(4, lvl)), set = new Set(); while (set.size < n) set.add(Math.floor(Math.random() * cells));
      let clicks = n + 7, found = 0;
      const decor = [ICO('wheat'), ICO('rock'), ICO('box'), ICO('trash'), ICO('nut')];
      this.box.innerHTML = '<div class="mg-find"></div>'; const g = $('.mg-find', this.box);
      for (let i = 0; i < cells; i++) {
        const e = h(`<button class="cell">${decor[Math.floor(Math.random() * decor.length)]}</button>`);
        e.onclick = () => { if (this.done || e.disabled) return; e.disabled = true; clicks--; if (set.has(i)) { found++; e.classList.add('hit'); e.innerHTML = ICO('star'); if (found >= n) return this.finish(true); } else e.classList.add('miss'); if (clicks <= 0) return this.finish(false); this.status(`Найдено: ${found}/${n} · осталось попыток: ${clicks}`); };
        g.append(e);
      }
      this.status(`Найдено: 0/${n} · попыток: ${clicks}`);
    }
  };
  const _mc = S.mini.close; S.mini.close = function () { if (this._ku) { document.removeEventListener('keyup', this._ku); this._ku = null; } _mc.call(this); };

  // ======================= главное меню /mm =======================
  S.mm = {
    open(d) {
      const tiles = [
        ['inv', 'bag', 'Инвентарь', 'I'], ['phone', 'phone', 'Телефон', 'P'], ['quest', 'scroll', 'Квесты', d.qa ? d.qa + ' акт.' : ''], ['jobs', 'briefcase', 'Работы', d.job || ''],
        ['gps', 'pin', 'GPS / Навигатор', ''], ['travel', 'ticket', 'Поезд / вокзалы', ''], ['cars', 'car', 'Мой транспорт', ''], ['ach', 'trophy', 'Достижения', ''],
        ['pass', 'idcard', 'Паспорт', d.pass ? 'есть' : 'нет'], ['medc', 'medkit', 'Медкарта', d.med ? 'есть' : 'нет'], ['lic', 'scroll', 'Лицензии', d.lic ? 'есть' : 'нет'], ['anims', 'hand', 'Анимации', ''],
        ['cmds', 'keyboard', 'Команды', ''], ['help', 'help', 'Помощь', ''], ['rules', 'info', 'Правила', ''], ['settings', 'wrench', 'Настройки', ''], ['report', 'chat', 'Репорт', 'админам']
      ];
      const pct = d.need ? Math.min(100, Math.round(d.exp * 100 / d.need)) : 0;
      const body = h(`<div class="mm">
        <div class="mm-prof">
          <div class="mm-ava">${ICO('user')}</div>
          <div class="grow"><b class="gold-t" style="font-size:18px">${esc(d.name)}</b> <span class="mut">ID ${d.id}</span>
            <div class="mm-lvl"><span>Уровень ${d.lvl}</span><div class="mg-bar fill sm"><i style="width:${pct}%"></i></div><span class="mut">${d.exp}/${d.need} EXP</span></div>
            <div class="mm-tags"><span>${ICO('cash')} ${money(d.cash)}</span><span>${ICO('bank')} ${money(d.bank)}</span>${d.debt ? `<span class="red-t">${ICO('sad')} Долг ${money(d.debt)}</span>` : ''}<span>${ICO('city')} ${esc(d.org || 'Без организации')}</span>${d.fam ? `<span>${ICO('users')} ${esc(d.fam)}</span>` : ''}<span>${ICO('phone')} ${esc(d.phone || '—')}</span></div>
          </div>
          <div class="mm-stat"><div><b>${d.hours | 0}</b><small>часов</small></div><div><b>${d.qd | 0}/${d.qt | 0}</b><small>квестов</small></div><div><b>${d.online | 0}</b><small>онлайн</small></div><div><b class="${d.warns ? 'red-t' : ''}">${d.warns | 0}/3</b><small>варны</small></div></div>
        </div>
        <div class="mm-grid"></div></div>`);
      const g = $('.mm-grid', body);
      tiles.forEach(([a, ic, t, s]) => { const e = h(`<button class="mm-t"><i>${ICO(ic)}</i><b>${t}</b><small>${esc(s)}</small></button>`); e.onclick = () => { close(); send('mm', a); }; g.append(e); });
      win('mmw', `<span class="logo">GODJO <b>RP</b></span> · Главное меню`, '/mm · ESC — закрыть', body, false);
    }
  };

  // ======================= затемнение экрана (поездки) =======================
  function onFade(d) {
    let f = $('#fade'); if (!f) { f = h('<div id="fade"><div></div></div>'); document.body.append(f); }
    $('div', f).textContent = d.text || ''; f.classList.add('on');
    clearTimeout(onFade.t); onFade.t = setTimeout(() => f.classList.remove('on'), Math.max(500, d.ms | 0));
  }

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
      case 'fade': onFade(d); break;
    }
    if (key === 'passport_done' || (key === 'queue' && d.done)) widget('w-queue', '');
  });

  // ======================= клавиатура =======================
  const isEsc = (e) => e.key === 'Escape' || e.key === 'Esc' || e.code === 'Escape' || e.keyCode === 27 || e.which === 27;
  let escAt = 0;
  function onEsc(e) {
    const now = Date.now(); if (now - escAt < 250) return; escAt = now; // keydown+keyup не закрывают дважды
    if (e.preventDefault) e.preventDefault();
    if (ctxEl) return closeCtx();
    if (cur && !(S[cur].noClose)) close();
    chatOpen = false;
  }
  window.gjEsc = () => onEsc({}); // для вызова из клиента (ASI/лаунчер)
  document.addEventListener('keyup', (e) => { if (isEsc(e) && cur) onEsc(e); }, true);
  document.addEventListener('keydown', (e) => {
    const typing = /INPUT|TEXTAREA|SELECT/.test((document.activeElement || {}).tagName || '');
    if (isEsc(e)) return onEsc(e);
    if (cur === 'mini' && S.mini.key(e)) { e.preventDefault(); return; }
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
    f('gj:hud', JSON.stringify({ cash: 1250, bank: 15400, hunger: 64, fatigue: 12, hp: 87, ar: 0, lvl: 3, exp: 5, need: 10, wanted: 2, speed: 87, fuel: 46, time: '21:37', id: 4, online: 57, det: 63, wep: 24, ammo: 35 }));
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
      'y': ['settings', {}],
      'u': ['mini', { type: 'wires', lvl: 2, token: 1, title: 'Ремонт щитка', hint: 'Соедините провода одного цвета' }],
      'i': ['mini', { type: 'seq', lvl: 2, token: 1, title: 'Граффити', hint: 'Повторите стрелки' }],
      'o': ['mini', { type: 'eye', lvl: 2, token: 1, title: 'Проверка зрения', hint: 'Буква под указкой' }],
      'p': ['mm', { name: 'Ivan_Petrov', id: 4, lvl: 3, exp: 5, need: 12, cash: 1250, bank: 15400, debt: 0, org: 'LSPD', fam: '', job: 'Таксист', phone: '5551234', hours: 12, pass: 1, med: 0, lic: 1, qa: 2, qd: 5, qt: 26, online: 57, warns: 0 }],
      't': ['prompt', { id: 'demo', title: 'Письмо', text: 'Пример окна подтверждения', input: true, ph: 'Введите текст', ok: 'Отправить' }]
    };
    console.log('Демо: клавиши 1-0, q, w, e, r, t, y, u, i, o, p открывают экраны; Esc закрывает');
    document.addEventListener('keydown', (e) => { if (cur || /INPUT|TEXTAREA|SELECT/.test(document.activeElement.tagName)) return; const d = demos[e.key]; if (d) open(d[0], d[1]); });
    if (location.hash.length > 1 && demos[location.hash.slice(1)]) { const d = demos[location.hash.slice(1)]; open(d[0], d[1]); }
    setTimeout(() => notify('ok', 'Добро пожаловать на Godjo Role Play!'), 300);
  }
})();
