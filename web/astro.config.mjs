import { defineConfig } from 'astro/config';
import mdx from '@astrojs/mdx';
import tailwind from '@astrojs/tailwind';

// https://astro.build/config
export default defineConfig({
  site: 'https://kudig-io.github.io',
  base: '/kudig-database',
  outDir: '../site',
  integrations: [
    mdx(),
    tailwind({
      applyBaseStyles: false,
    }),
  ],
  markdown: {
    shikiConfig: {
      theme: 'github-dark',
      wrap: true,
    },
    remarkPlugins: [],
    rehypePlugins: [],
  },
  vite: {
    server: {
      fs: {
        allow: ['..'],
      },
    },
  },
});
