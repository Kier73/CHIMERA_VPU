
import React from 'react';

interface SectionProps {
  title: string;
  children: React.ReactNode;
  className?: string;
  titleClassName?: string;
}

const Section: React.FC<SectionProps> = ({ title, children, className = '', titleClassName = '' }) => {
  return (
    <section className={`py-8 px-4 md:px-8 bg-white shadow-lg rounded-lg mb-12 ${className}`}>
      <h2 className={`text-3xl font-bold text-primary mb-6 pb-2 border-b-2 border-accent ${titleClassName}`}>
        {title}
      </h2>
      <div className="space-y-4 text-text-secondary text-lg">
        {children}
      </div>
    </section>
  );
};

export default Section;
