// End-to-end browser test: load qeynos2 from served test files, draw it,
// build a navmesh, save it. Usage: node browser_test.js <url> <outdir>
const { chromium } = require('playwright');
const fs = require('fs');
const path = require('path');

(async () => {
  const url = process.argv[2] || 'http://127.0.0.1:8765/index.html';
  const out = process.argv[3] || '/tmp/meshgen-test';
  fs.mkdirSync(out, { recursive: true });
  const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const page = await browser.newPage({ viewport: { width: 1400, height: 850 }, acceptDownloads: true });
  const logs = [];
  page.on('console', m => logs.push(`[${m.type()}] ${m.text()}`));
  page.on('pageerror', e => logs.push(`[pageerror] ${e.message}`));
  process.on('exit', () => { try { fs.writeFileSync(path.join(out, 'console.log'), logs.join('\n')); } catch (e) {} });

  await page.goto(url);
  await page.waitForFunction(() => window.meshgenReady, null, { timeout: 60000 });
  console.log('crossOriginIsolated:', await page.evaluate(() => crossOriginIsolated));
  await page.screenshot({ path: path.join(out, '0-start.png') });

  // Hand the page the zone files as if picked from an EverQuest folder.
  await page.evaluate(async () => {
    const names = ['qeynos2.s3d', 'qeynos2_obj.s3d'];
    const files = [];
    for (const n of names) {
      const r = await fetch('testzone/' + n);
      const b = await r.blob();
      files.push(new File([b], n));
    }
    Module.setEverQuestFiles(files, 'EverQuest');
    Module.ccall('meshgen_web_open_zone', null, ['string', 'number'], ['qeynos2', 0]);
  });
  await page.waitForFunction(() => Module.ccall('meshgen_web_status', 'number', [], []) >= 1, null, { timeout: 60000 });
  await page.waitForTimeout(1500);
  await page.screenshot({ path: path.join(out, '1-zone.png') });

  const t0 = Date.now();
  await page.evaluate(() => Module.ccall('meshgen_web_build', null, [], []));
  await page.waitForTimeout(300);
  await page.waitForFunction(() => Module.ccall('meshgen_web_status', 'number', [], []) === 3, null, { timeout: 300000 });
  console.log('build ms:', Date.now() - t0);
  await page.waitForTimeout(1500);
  await page.screenshot({ path: path.join(out, '2-built.png') });

  const dl = page.waitForEvent('download', { timeout: 30000 });
  await page.evaluate(() => Module.ccall('meshgen_web_save', null, [], []));
  const d = await dl;
  const dest = path.join(out, d.suggestedFilename());
  await d.saveAs(dest);
  console.log('downloaded:', dest, fs.statSync(dest).size, 'bytes');

  fs.writeFileSync(path.join(out, 'console.log'), logs.join('\n'));
  await browser.close();
})().catch(e => { console.error(e); process.exit(1); });
process.on('exit', () => {});
