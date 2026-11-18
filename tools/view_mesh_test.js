// Checks opening a mesh handed in by URL (?mesh=&zone=), as the site's
// moderation pages do. Usage: node view_mesh_test.js <base url> <mesh path> <outdir>
const { chromium } = require('playwright');
(async () => {
  const base = process.argv[2] || 'http://127.0.0.1:8765/';
  const mesh = process.argv[3] || '/files/qeynos2.navmesh';
  const out = process.argv[4] || '/tmp';
  const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const page = await browser.newPage({ viewport: { width: 1400, height: 850 } });
  page.on('pageerror', e => console.log('[pageerror]', e.message));
  await page.goto(base + 'index.html?zone=qeynos2&label=Qeynos2%20mesh%20v3&mesh=' + encodeURIComponent(mesh));
  await page.waitForFunction(() => window.meshgenReady, null, { timeout: 60000 });
  console.log('note:', await page.textContent('#view-note'));
  await page.screenshot({ path: out + '/view-0.png' });
  await page.evaluate(async () => {
    const files = [];
    for (const n of ['qeynos2.s3d', 'qeynos2_obj.s3d']) {
      const file = new File([await (await fetch('testzone/' + n)).blob()], n);
      files.push({ kind: 'file', name: n, getFile: async () => file });
    }
    Module.setEverQuestFiles(files, 'EverQuest');
  });
  await page.waitForFunction(() => Module.ccall('meshgen_web_status', 'number', [], []) === 3, null, { timeout: 60000 });
  await page.waitForTimeout(1500);
  await page.screenshot({ path: out + '/view-1.png' });
  console.log('mesh loaded over the zone (status 3)');
  await browser.close();
})().catch(e => { console.error(e); process.exit(1); });
