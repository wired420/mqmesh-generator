// Checks that a picked EverQuest folder is remembered across visits. Uses an
// OPFS directory as a stand-in for the real folder (a real directory handle
// that can be stored in IndexedDB). Usage: node saved_folder_test.js <url>
const { chromium } = require('playwright');

(async () => {
  const url = process.argv[2] || 'http://127.0.0.1:8765/index.html';
  const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const ctx = await browser.newContext({ viewport: { width: 1200, height: 800 } });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('[pageerror]', e.message));

  await page.goto(url);
  await page.waitForFunction(() => window.meshgenReady, null, { timeout: 60000 });
  // Fill an OPFS folder with the zone files and make the picker return it.
  await page.evaluate(async () => {
    const root = await navigator.storage.getDirectory();
    const dir = await root.getDirectoryHandle('EverQuest', { create: true });
    for (const n of ['qeynos2.s3d', 'qeynos2_obj.s3d']) {
      const b = await (await fetch('testzone/' + n)).blob();
      const w = await (await dir.getFileHandle(n, { create: true })).createWritable();
      await w.write(b); await w.close();
    }
    window.showDirectoryPicker = async () => dir;
  });
  await page.click('#pick');
  await page.waitForFunction(() => document.getElementById('overlay').classList.contains('hidden'), null, { timeout: 30000 });
  console.log('first visit: folder picked');

  // Second visit: should open the saved folder without a click.
  await page.reload();
  await page.waitForFunction(() => window.meshgenReady, null, { timeout: 60000 });
  await page.waitForFunction(() => document.getElementById('overlay').classList.contains('hidden'), null, { timeout: 15000 });
  await page.evaluate(() => Module.ccall('meshgen_web_open_zone', null, ['string', 'number'], ['qeynos2', 0]));
  await page.waitForFunction(() => Module.ccall('meshgen_web_status', 'number', [], []) >= 1, null, { timeout: 60000 });
  console.log('second visit: saved folder opened automatically, qeynos2 loaded');

  // Forget it: the next visit shows the picker again.
  await page.evaluate(() => Module.forgetSavedFolder());
  await page.reload();
  await page.waitForFunction(() => window.meshgenReady, null, { timeout: 60000 });
  await page.waitForTimeout(1000);
  const shown = await page.evaluate(() => !document.getElementById('overlay').classList.contains('hidden') && document.getElementById('saved').hidden);
  console.log('after forget: picker shown, no saved button:', shown);
  await browser.close();
  if (!shown) process.exit(1);
})().catch(e => { console.error(e); process.exit(1); });
